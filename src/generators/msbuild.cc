#include "generators/msbuild.hh"
#include <openssl/md5.h>
#include <array>
#include <base/utils.hh>
#include <fstream>
#include <iostream>
#include <set>

#ifdef _WIN32
#include "win32/vssetup.hh"
#endif

using namespace std::literals;

namespace {
	std::string_view varname(var v) {
		switch (v) {
			case var::INPUT:
				return "$in"sv;
			case var::OUTPUT:
				return "$out"sv;
			case var::MAIN_OUTPUT:
				return "$MAIN_OUTPUT"sv;
			case var::LINK_FLAGS:
				return "$LINK_FLAGS"sv;
			case var::LINK_PATH:
				return "$LINK_PATH"sv;
			case var::LINK_LIBRARY:
				return "$LINK_LIBRARY"sv;
			case var::DEFINES:
				return "$DEFINES"sv;
			case var::CFLAGS:
				return "$CFLAGS"sv;
			case var::CXXFLAGS:
				return "$CXXFLAGS"sv;
		}
		return {};
	}

	std::string_view name2sv(rule_name const& name) {
		return std::visit(
		    [](auto const& name) -> std::string_view {
			    if constexpr (std::is_same_v<decltype(name),
			                                 std::monostate const&>)
				    return "[src]"sv;
			    else if constexpr (std::is_same_v<decltype(name),
			                                      std::string const&>)
				    return name;
			    else {
				    return {};
			    }
		    },
		    name);
	}

	std::u8string filename_from(std::filesystem::path const& back_to_sources,
	                            file_ref const& file,
	                            std::vector<project_setup> const& setups) {
		// input: .. / subdir / filename
		// output: subdir / objdir / filename
		// linked: subdir / filename
		using std::filesystem::path;
		auto& setup = setups[file.prj];
		switch (file.type) {
			case file_ref::input:
				return (back_to_sources / setup.subdir / file.path)
				    .generic_u8string();
			case file_ref::output:
				return (path{setup.subdir} / setup.objdir / file.path)
				    .generic_u8string();
			case file_ref::linked:
				return (path{setup.subdir} / file.path).generic_u8string();
			case file_ref::include:
			case file_ref::header_module:
				break;
		}
		return file.path;
	}

	void print_artifact(artifact const& name,
	                    std::filesystem::path const& back_to_sources,
	                    std::vector<project_setup> const& setups) {
		std::visit(
		    [&](auto const& name) {
			    if constexpr (std::is_same_v<decltype(name), file_ref>) {
				    std::cout << filename_from(back_to_sources, name, setups);
			    } else if constexpr (std::is_same_v<decltype(name), mod_ref>) {
			    }
		    },
		    name);
	}

	std::string_view name_of(project::kind kind) {
		switch (kind) {
			case project::executable:
				return "Application"sv;
			case project::static_lib:
				return "StaticLibrary"sv;
			case project::shared_lib:
				return "DynamicLibrary"sv;
		}
		return "Utility"sv;
	}

	struct Guids {
		explicit Guids(std::filesystem::path const& binary_dir)
		    : bindir_{binary_dir.generic_u8string()} {}

		std::string const& getGuid(std::u8string const& name) {
			auto it = guids_.lower_bound(name);
			if (it == guids_.end() || it->first != name) {
				auto const payload = bindir_ + u8"|" + name;
				it = guids_.insert(it, {name, uuid3(payload)});
			}
			return it->second;
		}

	private:
		std::string uuid3(std::u8string const& payload) {
			static constexpr auto ns =
			    "\xee\x30\xc4\xbe\x51\x92\x4f\xb0\xb3\x35\x72\x2a\x2d\xff\xe7\x60"sv;
			unsigned char uuid[MD5_DIGEST_LENGTH];
			MD5_CTX ctx{};
			MD5_Init(&ctx);
			MD5_Update(&ctx, ns.data(), ns.size());
			MD5_Update(&ctx, payload.data(), payload.size());
			MD5_Final(uuid, &ctx);

			uuid[6] &= 0xF;
			uuid[6] |= static_cast<unsigned char>(3u << 4);

			uuid[8] &= 0x3F;
			uuid[8] |= 0x80;

			std::string output{};
			output.reserve(32 + 4);
			static constexpr std::array<int, 5> uuid_groups{4, 2, 2, 2, 6};
			static constexpr char alphabet[] = "0123456789ABCDEF";

			size_t input_index = 0;
			for (auto const group : uuid_groups) {
				if (input_index) output += '-';

				for (int pos = 0; pos < group; ++pos) {
					auto const byte = uuid[input_index++];
					output += alphabet[(byte >> 4) & 0xF];
					output += alphabet[(byte >> 0) & 0xF];
				}
			}

			return output;
		}

		std::u8string bindir_{};
		std::map<std::u8string, std::string> guids_{};
	};

	struct VsSource {
		file_ref file;
		std::u8string exports;
	};

	struct VsProject {
		artifact name;
		std::string guid;
		project::kind kind;
		std::vector<VsSource> sources;
		std::vector<std::string> ref_guids;
	};
}  // namespace

void msbuild::generate(std::filesystem::path const& back_to_sources,
                       std::filesystem::path const& binary_dir) {
	std::map<artifact, std::u8string> interfaces{};
	for (auto const& target : targets_) {
		if (!std::holds_alternative<std::monostate>(target.rule)) continue;
		if (!target.edge.empty()) {
			interfaces[target.main_output] = target.edge;
		}
	}

	std::map<std::string, VsProject> projects{};
	Guids guids{binary_dir};

	for (auto const& target : targets_) {
		if (!std::holds_alternative<std::string>(target.rule)) continue;
		auto const& rule = std::get<std::string>(target.rule);

		project::kind kind{};

		if (rule == "VS-EXE"sv)
			kind = project::executable;
		else if (rule == "VS-LIB"sv)
			kind = project::static_lib;
		else if (rule == "VS-DLL"sv)
			kind = project::shared_lib;
		else
			continue;

		auto guid =
		    guids.getGuid(filename(back_to_sources, target.main_output));

		auto& prj = projects[guid];
		prj.name = target.main_output;
		prj.kind = kind;
		prj.guid = std::move(guid);
		prj.sources.reserve(target.inputs.expl.size());
		prj.ref_guids.reserve(target.inputs.impl.size());

		for (auto const& src : target.inputs.expl) {
			if (!std::holds_alternative<file_ref>(src)) continue;
			auto it = interfaces.find(src);
			auto& ref = std::get<file_ref>(src);
			if (it != interfaces.end()) {
				prj.sources.push_back({ref, it->second});
			} else {
				prj.sources.push_back({ref, {}});
			}
		}

		for (auto const& lib : target.inputs.impl) {
			prj.ref_guids.push_back(
			    guids.getGuid(filename(back_to_sources, lib)));
		}
	}

	for (auto const& [_, prj] : projects) {
		auto const prjname = filename(back_to_sources, prj.name);
		auto const dirname = binary_dir / (prjname + u8".dir"s);
		auto const additional_back = [&] {
			size_t count{};
			for (auto c : prjname) {
				if (c == '/') ++count;
			}
			std::string result{};
			result.reserve(count * 3);
			for (size_t index = 0; index < count; ++index)
				result.append("../"sv);
			return result;
		}();

		std::error_code ec{};
		std::filesystem::create_directories(dirname, ec);
		if (ec) {
			std::cerr << "c++modules: cannot create "
			          << as_sv(binary_dir.generic_u8string()) << ": "
			          << ec.message() << '\n';
			std::exit(1);
		}

		{
			std::ofstream vcxproj{binary_dir / (prjname + u8".vcxproj"s)};
			vcxproj << R"(<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="17.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <PreferredToolArchitecture>x64</PreferredToolArchitecture>
)"sv;

			for (auto const& platform : {"x64"sv}) {
				for (auto const& config : {"Debug"sv, "Release"sv}) {
					;
					vcxproj
					    << R"(    <OutDir Condition="'$(Configuration)|$(Platform)'==')"sv
					    << config << R"(|)"sv << platform << R"('">)"sv
					    << as_sv((std::filesystem::path{additional_back} /
					              platform / config /
					              std::filesystem::path{prjname}.parent_path())
					                 .make_preferred()
					                 .u8string())
					    <<
					    R"(\</OutDir>
    <IntDir Condition="'$(Configuration)|$(Platform)'==')"sv
					    << config << R"(|)"sv << platform << R"('">)"sv
					    << as_sv((std::filesystem::path{prjname + u8".dir"}
					                  .filename() /
					              platform / config)
					                 .make_preferred()
					                 .u8string())
					    << R"(\</IntDir>
)"sv;
				}
			}

			vcxproj << R"(  </PropertyGroup>
  <ItemGroup Label="ProjectConfigurations">
)"sv;

			for (auto const& platform : {"x64"sv}) {
				for (auto const& config : {"Debug"sv, "Release"sv}) {
					vcxproj << R"(    <ProjectConfiguration Include=")"sv
					        << config << R"(|)"sv << platform <<
					    R"(">
      <Configuration>)"sv << config
					        <<
					    R"(</Configuration>
      <Platform>)"sv << platform
					        <<
					    R"(</Platform>
    </ProjectConfiguration>
)"sv;
				}
			}

			vcxproj << R"(  </ItemGroup>
  <PropertyGroup Label="Globals">
    <ProjectGuid>{)"sv
			        << prj.guid << R"(}</ProjectGuid>
)"sv;
#ifdef _WIN32
			auto const win10 = vssetup::win10sdk();
			if (!win10.empty()) {
				vcxproj << R"(    <WindowsTargetPlatformVersion>)"sv << win10
				        << R"(</WindowsTargetPlatformVersion>
)"sv;
			}
#endif

			vcxproj << R"(    <Keyword>Win32Proj</Keyword>
    <Platform>x64</Platform>
    <ProjectName>)"sv
			        << as_sv(std::filesystem::path{
			               filename(back_to_sources, prj.name)}
			                     .filename()
			                     .generic_u8string())
			        <<
			    R"(</ProjectName>
    <VCProjectUpgraderObjectName>NoUpgrade</VCProjectUpgraderObjectName>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Label="Configuration">
    <ConfigurationType>)"sv
			        << name_of(prj.kind) << R"prefix(</ConfigurationType>
    <CharacterSet>Unicode</CharacterSet>
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
    <UseDebugLibraries>false</UseDebugLibraries>
    <WholeProgramOptimization>true</WholeProgramOptimization>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
    <UseDebugLibraries>true</UseDebugLibraries>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Label="Shared">
  </ImportGroup>
  <ImportGroup Label="PropertySheets">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <PropertyGroup Label="UserMacros" />
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp20</LanguageStandard>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <EnableCOMDATFolding>true</EnableCOMDATFolding>
      <OptimizeReferences>true</OptimizeReferences>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemGroup>
)prefix"sv;

			for (auto const& src : prj.sources) {
				auto path = filename_from(back_to_sources, src.file, setups_);

				if (src.exports.empty()) {
					vcxproj << R"(    <ClCompile Include=")"sv
					        << additional_back << as_sv(path) << R"(" />
)"sv;
				} else {
					vcxproj << R"(    <ClCompile Include=")"sv
					        << additional_back << as_sv(path) << R"(">
      <CompileAs>CompileAsCppModule</CompileAs>
    </ClCompile>
)"sv;
				}
			}

			vcxproj << R"(  </ItemGroup>
  <ItemGroup>
)"sv;

			for (auto const& ref : prj.ref_guids) {
				auto it = projects.find(ref);
				if (it == projects.end()) continue;
				auto const& dep = it->second;
				auto path = binary_dir / filename(back_to_sources, dep.name);
				path.make_preferred();

				vcxproj << R"(    <ProjectReference Include=")"sv
				        << as_sv(path.u8string()) << R"(.vcxproj">
      <Project>{)"sv << ref
				        << R"(}</Project>
      <Name>)"sv << as_sv(filename(back_to_sources, dep.name))
				        << R"(</Name>
    </ProjectReference>
)"sv;
			}

			vcxproj << R"(  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets">
  </ImportGroup>
</Project>
)"sv;
		}
	}

	{
		std::ofstream sln{binary_dir / u8"scanned.sln"sv};
		sln << R"(Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 17
)"sv;

		for (auto const& [_, prj] : projects) {
			auto const prjname =
			    std::filesystem::path{filename(back_to_sources, prj.name)}
			        .make_preferred()
			        .u8string();
			sln << R"(Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = ")"sv
			    << as_sv(std::filesystem::path{prjname}.filename().u8string())
			    << R"(", ")"sv << as_sv(prjname) << R"(.vcxproj", "{)"sv
			    << prj.guid << R"(}"
	ProjectSection(ProjectDependencies) = postProject
)"sv;
			for (auto const& guid : prj.ref_guids) {
				sln << R"(		{)"sv << guid << R"(} = {)"sv << guid << R"(}
)"sv;
			}
			sln << R"(	EndProjectSection
EndProject
)"sv;
		}

		sln << R"(Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|x64 = Debug|x64
		Release|x64 = Release|x64
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
)"sv;
		for (auto const& [guid, _] : projects) {
			for (auto const& platform : {"x64"sv}) {
				for (auto const& config : {"Debug"sv, "Release"sv}) {
					sln << R"(		{)"sv << guid << R"(}.)"sv << config
					    << R"(|)"sv << platform << R"(.ActiveCfg = )"sv
					    << config << R"(|)"sv << platform << R"(
		{)"sv << guid << R"(}.)"sv
					    << config << R"(|)"sv << platform
					    << R"(.Build.0 = )"sv << config << R"(|)"sv
					    << platform << R"(
)"sv;
				}
			}
		}

		sln << R"(	EndGlobalSection
	GlobalSection(NestedProjects) = preSolution
	EndGlobalSection
	GlobalSection(ExtensibilityGlobals) = postSolution
	EndGlobalSection
	GlobalSection(ExtensibilityAddIns) = postSolution
	EndGlobalSection
EndGlobal
)"sv;
	}
}

std::u8string msbuild::filename(std::filesystem::path const& back_to_sources,
                                artifact const& fileref) {
	return std::visit(
	    [&](auto const& file) -> std::u8string {
		    if constexpr (std::is_same_v<decltype(file), file_ref const&>) {
			    return filename_from(back_to_sources, file, setups_);
		    } else {
			    return file.path;
		    }
	    },
	    fileref);
}