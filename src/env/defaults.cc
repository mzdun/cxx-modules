#include <env/defaults.hh>

using namespace std::literals;

namespace env {
	affix::path affix::modify(path const& file) const {
		auto const filename = file.filename().u8string();
		auto const dirname = file.parent_path();
		std::u8string result{};
		result.reserve(filename.length() + prefix.length() + suffix.length());
		result.append(prefix);
		result.append(filename);
		result.append(suffix);
		return dirname / path{std::move(result)};
	}

	modifiers const& path_mods() noexcept {
		static modifiers mods{
#ifdef _WIN32
		    .executable = {{}, u8".exe"sv},
		    .static_library = {{}, u8".lib"sv},
		    .shared_library = {{}, u8".dll"sv},
		    .object = {{}, u8".obj"sv},
#else
		    .executable = {{}, {}},
		    .static_library = {u8"lib"sv, u8".a"sv},
		    .shared_library = {u8"lib"sv, u8".so"sv},
		    .object = {{}, u8".o"sv},
#endif
		};
		return mods;
	}
}  // namespace env
