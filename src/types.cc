#include "types.hh"
#include <fs/file.hh>
#include <iostream>
#include <json/json.hpp>
#include "compiler.hh"
#include "scanner.hh"
#include "utils.hh"

namespace {
	void load_directory(std::map<project, project::setup>& result,
	                    fs::path const& current,
	                    fs::path const& source_dir) {
		std::error_code ec{};
		auto subdir = fs::relative(current, source_dir, ec);
		if (ec) subdir = current;
		if (subdir == "."sv) subdir = fs::path{};

		auto data = [&current] {
			auto json_file = fs::fopen(current / "sources.json");
			if (!json_file) {
				std::cerr << "c++modules: cannot open sources.json inside "
				          << current.generic_string() << "\n";
				std::exit(1);
			}

			auto bytes = json_file.read();
			return json::read_json(
			    {reinterpret_cast<char8_t const*>(bytes.data()), bytes.size()});
		}();

		auto json_projects = cast<json::map>(data);
		if (!json_projects) return;

		for (auto const& [key, node] : *json_projects) {
			if (key == u8".dirs"sv) {
				auto json_subdirs = cast<json::array>(node);
				if (!json_subdirs) continue;

				for (auto& json_item : *json_subdirs) {
					auto item = cast<json::string>(json_item);
					if (!item) continue;
					load_directory(result, current / *item, source_dir);
				}
				continue;
			}

			auto json_project = cast<json::map>(node);
			if (!json_project) continue;
			auto json_type =
			    cast_from_json<json::string>(json_project, u8"type"sv);
			auto json_sources =
			    cast_from_json<json::array>(json_project, u8"sources"sv);
			if (!json_type || !json_sources) continue;
			project pro{key};
			if (*json_type == u8"executable"sv)
				pro.type = project::executable;
			else if (*json_type == u8"static"sv)
				pro.type = project::static_lib;
			else if (*json_type == u8"shared"sv)
				pro.type = project::shared_lib;
			else if (*json_type == u8"module"sv)
				pro.type = project::module_lib;
			else
				continue;

			std::vector<std::filesystem::path> sources;
			sources.reserve(json_sources->size());

			for (auto& json_item : *json_sources) {
				auto item = cast<json::string>(json_item);
				if (!item) continue;
				sources.emplace_back(*item);
			}

			if (sources.empty()) continue;

			result[std::move(pro)] = project::setup{
			    subdir.generic_u8string(),
			    std::move(sources),
			};
		}
	}
}  // namespace

std::map<project, project::setup> project::load(fs::path const& source_dir) {
	std::map<project, setup> result{};
	load_directory(result, source_dir, source_dir);
	return result;
}

std::u8string mod_name::toString() const {
	std::u8string result{};
	auto size = module.size() + part.size();
	if (!part.empty()) ++size;
	result.reserve(size);
	result.append(module);
	if (!part.empty()) {
		result.push_back(':');
		result.append(part);
	}
	return result;
}

std::u8string mod_name::toBMI() const {
	std::u8string result{};
	auto size = module.size() + part.size() + 4;
	if (!part.empty()) ++size;
	result.reserve(size);
	result.append(module);
	if (!part.empty()) {
		result.push_back('-');
		result.append(part);
	}
	result.append(u8".bmi"sv);
	return result;
}

static build_info normalized_paths(std::filesystem::path const& source_dir,
                                   std::filesystem::path const& build_dir) {
	auto normalized = [](fs::path const& path) -> fs::path {
		std::error_code ec;
		auto abs = fs::absolute(path, ec);
		if (!ec) return abs.lexically_normal();
		auto canonical = fs::weakly_canonical(path, ec);
		if (!ec) return canonical.lexically_normal();
		return path.lexically_normal();
	};

	return {normalized(source_dir).generic_u8string(),
	        normalized(build_dir).generic_u8string()};
}

build_info build_info::analyze(
    std::map<project, project::setup> const& projects,
    compiler_info const& cxx,
    std::filesystem::path const& source_dir,
    std::filesystem::path const& build_dir) {
	auto build = normalized_paths(source_dir, build_dir);

	for (auto const& [project, setup] : projects) {
		auto& dependency = build.projects[project];
		dependency.subdir = setup.subdir;
		dependency.sources.reserve(setup.sources.size());
		std::transform(setup.sources.begin(), setup.sources.end(),
		               std::back_inserter(dependency.sources),
		               [](fs::path const& p) { return p.generic_u8string(); });

		for (auto const& source : setup.sources) {
			auto srcfile =
			    (source_dir / setup.subdir / source).lexically_normal();
			auto const text = cxx.preproc(srcfile);
			if (!text) continue;

			auto unit = scan(*text);

			auto u8path =
			    (setup.subdir / source).lexically_normal().generic_u8string();

			if (!unit.name.empty()) {
				if (unit.is_interface) {
					build.modules[unit.name].interface = u8path;
					build.exports[u8path] = unit.name;
					dependency.exports.insert(unit.name);
				}
			}
			build.modules[unit.name].libs.insert(project);
			if (!unit.is_interface)
				build.modules[unit.name].sources.push_back(u8path);

			for (auto& import : unit.imports) {
				if (unit.name != import)
					build.modules[unit.name].req.insert(import);
				dependency.imports.insert(import);
				build.imports[u8path].push_back(import);
			}
		}

		std::vector<mod_name> imports{dependency.imports.begin(),
		                              dependency.imports.end()};
		auto it = std::remove_if(
		    imports.begin(), imports.end(), [&](mod_name const& mod) {
			    bool found = false;
			    for (auto const& rhs : dependency.exports) {
				    if (rhs == mod) {
					    found = true;
					    break;
				    }
			    }
			    return found;
		    });

		dependency.imports = std::set<mod_name>{imports.begin(), it};
	}

	// turn project level imports into link dependencies
	for (auto& [prj, deps] : build.projects) {
		std::vector<mod_name> imports{deps.imports.begin(), deps.imports.end()};

		auto it = std::remove_if(
		    imports.begin(), imports.end(),
		    [&ref = deps, &build](mod_name const& mod) {
			    bool found = false;
			    for (auto const& [rhs_prj, rhs_deps] : build.projects) {
				    for (auto const& rhs : rhs_deps.exports) {
					    if (rhs == mod) {
						    // ok, this library provides a part of module, note
						    // that down and carry on
						    found = true;
						    ref.links.insert(rhs_prj);
						    break;
					    }
				    }
			    }
			    return found;
		    });

		deps.imports = std::set<mod_name>{imports.begin(), it};
	}

	return build;
}

fs::path build_info::source_from_build() const {
	std::error_code ec;
	auto sourcedir = fs::relative(source_dir, build_dir, ec);
	if (ec) sourcedir = source_dir;
	return sourcedir.lexically_normal();
}
