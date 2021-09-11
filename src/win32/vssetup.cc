#include "vssetup.hh"

#include <winsdkver.h>

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x0601

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <comdef.h>
#include <comip.h>
#include <comutil.h>

#include <fstream>
#include <iostream>
#include <vector>

#include <base/utils.hh>
#include "Setup.Configuration.h"

_COM_SMARTPTR_TYPEDEF(ISetupInstance, __uuidof(ISetupInstance));
_COM_SMARTPTR_TYPEDEF(ISetupInstance2, __uuidof(ISetupInstance2));
_COM_SMARTPTR_TYPEDEF(IEnumSetupInstances, __uuidof(IEnumSetupInstances));
_COM_SMARTPTR_TYPEDEF(ISetupConfiguration, __uuidof(ISetupConfiguration));
_COM_SMARTPTR_TYPEDEF(ISetupConfiguration2, __uuidof(ISetupConfiguration2));
_COM_SMARTPTR_TYPEDEF(ISetupHelper, __uuidof(ISetupHelper));
_COM_SMARTPTR_TYPEDEF(ISetupPackageReference, __uuidof(ISetupPackageReference));
_COM_SMARTPTR_TYPEDEF(ISetupPropertyStore, __uuidof(ISetupPropertyStore));
_COM_SMARTPTR_TYPEDEF(ISetupInstanceCatalog, __uuidof(ISetupInstanceCatalog));

using namespace std::literals;

namespace vssetup {
	namespace {
		// <https://github.com/microsoft/vs-setup-samples/blob/main/Setup.Configuration.VC/Helpers.h>
		struct SafeArrayDeleter {
			void operator()(_In_ LPSAFEARRAY* ppsa) {
				if (ppsa && *ppsa) {
					if ((*ppsa)->cLocks) {
						::SafeArrayUnlock(*ppsa);
					}

					::SafeArrayDestroy(*ppsa);
				}
			}
		};

		typedef std::unique_ptr<LPSAFEARRAY, SafeArrayDeleter> safearray_ptr;

		class win32_exception : public std::exception {
		public:
			win32_exception(HRESULT code, const char* what) noexcept
			    : std::exception(what), m_code(code) {}

			win32_exception(const win32_exception& obj) noexcept
			    : std::exception(obj) {
				m_code = obj.m_code;
			}

			HRESULT code() const noexcept { return m_code; }

		private:
			HRESULT m_code;
		};

		class CoInitializer {
		public:
			CoInitializer() {
				hr = ::CoInitialize(NULL);
				if (FAILED(hr)) {
					throw win32_exception(hr, "failed to initialize COM");
				}
			}

			~CoInitializer() {
				if (SUCCEEDED(hr)) {
					::CoUninitialize();
				}
			}

		private:
			HRESULT hr;
		};
		// </https://github.com/microsoft/vs-setup-samples/blob/main/Setup.Configuration.VC/Helpers.h>

		constexpr auto MASK = 0xFFFFull;
		static constexpr auto tooldirname = R"(VC\Tools\MSVC)"sv;
		static constexpr auto x64bin = R"(bin\Hostx64\x64)"sv;

		struct VSInstance {
			ULONGLONG vs_version{};
			std::filesystem::path root{};
			std::string vc_version{};
			auto operator<=>(VSInstance const&) const = default;
		};

#define COM(expr, message)                  \
	if (auto const hr = expr; FAILED(hr)) { \
		throw win32_exception(hr, message); \
	}

		ISetupConfigurationPtr create_setup_configuration() {
			ISetupConfigurationPtr query{};
			auto const hr = query.CreateInstance(__uuidof(SetupConfiguration));
			if (REGDB_E_CLASSNOTREG == hr) {
				// The query API is not registered. Assuming no
				// instances are installed
				return {};
			} else if (FAILED(hr)) {
				throw win32_exception(hr, "failed to create query class");
			}
			return query;
		}

		std::filesystem::path get_root(ISetupInstance2* instance) {
			bstr_t path_str;
			COM(instance->GetInstallationPath(path_str.GetAddress()),
			    "failed to get InstallationPath");
			return std::wstring_view{path_str, path_str.length()};
		}

		ULONGLONG vs_version(ISetupInstance2* instance, ISetupHelper* helper) {
			ULONGLONG result{};
			bstr_t version_str;
			COM(instance->GetInstallationVersion(version_str.GetAddress()),
			    "failed to get InstallationVersion");
			COM(helper->ParseVersion(version_str, &result),
			    "failed to parse InstallationVersion");
			return result;
		}

		std::string vc_version(std::filesystem::path const& root) {
			static constexpr auto ver_file =
			    R"(VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt)"sv;

			std::string result;
			std::ifstream file{root / ver_file};
			if (!file || !std::getline(file, result)) {
				return {};
			}
			result = strip_s(result);

			auto const dirname = root / tooldirname / result;
			std::error_code ec{};
			auto const status = std::filesystem::status(dirname, ec);
			if (ec || !std::filesystem::is_directory(status)) {
				return {};
			}

			return result;
		}

		VSInstance create_instance(ISetupInstance* base_instance,
		                           ISetupHelper* helper) {
			VSInstance result{};
			InstanceState state{};

			ISetupInstance2Ptr instance{base_instance};
			COM(instance->GetState(&state), "failed to get State");
			if ((state & eLocal) != eLocal) return result;

			result.root = get_root(instance);
			result.vs_version = vs_version(instance, helper);
			result.vc_version = vc_version(result.root);

			return result;
		}

		std::vector<VSInstance> enum_instalations() {
			std::vector<VSInstance> result{};

			try {
				CoInitializer init{};
				auto query = create_setup_configuration();
				if (!query) return result;

				ISetupConfiguration2Ptr query2(query);
				ISetupHelperPtr helper(query);
				IEnumSetupInstancesPtr e;

				COM(query2->EnumAllInstances(&e),
				    "failed to query all instances");

				ISetupInstance* pInstances[1] = {};
				auto hr = e->Next(1, pInstances, NULL);
				while (S_OK == hr) {
					// Wrap instance without AddRef'ing.
					ISetupInstancePtr instance(pInstances[0], false);
					auto inst = create_instance(instance, helper);
					if (!inst.root.empty() && inst.vs_version != 0 &&
					    !inst.vc_version.empty()) {
						result.push_back(std::move(inst));
					}

					hr = e->Next(1, pInstances, NULL);
				}

				if (FAILED(hr)) {
					throw win32_exception(hr,
					                      "failed to enumerate all instances");
				}

			} catch (win32_exception& ex) {
				std::cerr << std::hex << "Error 0x" << ex.code() << ": "
				          << ex.what() << '\n';
			} catch (_com_error& err) {
				std::wcerr << std::hex << L"Error 0x" << err.Error() << ": "
				           << err.ErrorMessage() << '\n';
			}

			std::sort(result.begin(), result.end(), std::greater<>{});
			return result;
		};

		std::vector<VSInstance> const& installations() {
			static auto instances = enum_instalations();
			return instances;
		}
	}  // namespace

	std::filesystem::path find_compiler() {
		for (auto const& inst : installations()) {
			return inst.root / tooldirname / inst.vc_version / x64bin /
			       "cl.exe"sv;
		}
		return "cl.exe"sv;
	}
}  // namespace vssetup