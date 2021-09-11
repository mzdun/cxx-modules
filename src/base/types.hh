#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std::literals;

struct project {
	enum kind { executable, static_lib, shared_lib, module_lib };
	static constexpr std::u8string_view kind2str[] = {u8"EXE"sv, u8"LIB"sv,
	                                                  u8"SO"sv, u8"MOD"sv};
	static constexpr std::u8string_view prefix[] = {u8""sv, u8"lib"sv,
	                                                u8"lib"sv, u8"lib"sv};
	static constexpr std::u8string_view suffix[] = {u8""sv, u8".a"sv, u8".so"sv,
	                                                u8".mod"sv};

	std::u8string name;
	kind type{static_lib};

	auto operator<=>(project const&) const = default;

	std::u8string filename() const {
		std::u8string result{};
		auto pre = prefix[type];
		auto ext = suffix[type];
		result.reserve(name.size() + pre.size() + ext.size());
		result.append(pre);
		result.append(name);
		result.append(ext);
		return result;
	}

	struct setup {
		std::filesystem::path subdir;
		std::vector<std::filesystem::path> sources;
	};

	static std::map<project, setup> load(
	    std::filesystem::path const& source_dir);
};

struct mod_name {
	std::u8string module;
	std::u8string part;

	bool empty() const noexcept { return module.empty() && part.empty(); }
	std::u8string toString() const;
	std::u8string toBMI() const;
	auto operator<=>(mod_name const&) const = default;
};

struct module_unit {
	mod_name name{};
	std::vector<mod_name> imports{};
	bool is_interface{false};
};

struct module_info {
	std::u8string interface;
	std::vector<std::u8string> sources;
	std::set<mod_name> req;
	std::set<project> libs;
};

struct project_info {
	std::filesystem::path subdir;
	std::vector<std::u8string> sources;
	std::set<mod_name> exports;
	std::set<mod_name> imports;
	std::set<project> links;
};

struct build_info {
	std::u8string source_dir{};
	std::u8string binary_dir{};
	std::map<mod_name, module_info> modules{};
	std::map<project, project_info> projects{};
	std::map<std::u8string, std::vector<mod_name>> imports{};
	std::map<std::u8string, mod_name> exports{};

	static build_info analyze(std::map<project, project::setup> const&,
	                          struct compiler_info const&,
	                          std::filesystem::path const&,
	                          std::filesystem::path const&);

	std::filesystem::path source_from_binary() const;
};