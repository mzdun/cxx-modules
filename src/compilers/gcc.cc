#include <charconv>
#include <sstream>
#include "compiler.hh"
#include "generator.hh"
#include "process.hpp"
#include "types.hh"
#include "utils.hh"

using namespace std::literals;

namespace {
	std::u8string as_cmi(mod_name const& name) {
		static constexpr auto cache = u8"gcm.cache/"sv;
		static constexpr auto ext = u8".gcm"sv;

		std::u8string fname{};
		fname.reserve(cache.size() + name.module.size() +
		              (name.part.empty() ? 0 : 1 + name.part.size()) +
		              ext.size());
		fname.append(cache);
		fname.append(name.module);
		if (!name.part.empty()) {
			fname.push_back('-');
			fname.append(name.part);
		}
		fname.append(ext);
		return fname;
	}
}  // namespace

class gcc : public compiler {
public:
	gcc(std::u8string_view exec, std::string_view, std::string_view version)
	    : compiler_{exec} {
		auto pos = version.find(' ');
		if (pos == std::string_view::npos) {
			major_ = gcc_with_modules;
		} else {
			auto const major_sv = version.substr(0, pos);
			auto const end = major_sv.data() + major_sv.size();
			auto result = std::from_chars(major_sv.data(), end, major_);
			if (result.ec != std::errc{} || result.ptr != end) major_ = 255;
		}
	}

	void mapout(build_info const& build, generator& gen) override;
	std::vector<templated_string> commands_for(rule_type) override;

private:
	std::string toolname(std::string_view tool) const {
		std::string result{}, major{};
		size_t length = tool.length() + prefix_.length();
		if (!prefix_.empty()) {
			major = std::to_string(major_);
			length += major.length() + 1;
		}
		result.reserve(length);
		result.append(as_sv(prefix_));
		result.append(tool);
		if (!major.empty()) {
			result.push_back('-');
			result.append(major);
		}
		return result;
	};

	std::u8string find_prefix() {
		auto const dirname = compiler_.parent_path();
		auto const filename = compiler_.filename();

		static constexpr auto dash = '-';
		auto& fname_str = filename.native();
		size_t prev = 0;
		for (auto _ = 0; _ < 3; ++_) {
			auto pos = fname_str.find(dash, prev);
			if (pos == fs::path::string_type::npos) return {};
			prev = pos + 1;
		}
		return (dirname / fname_str.substr(0, prev)).generic_u8string() +
		       u8"gcc-";
	}

	fs::path compiler_{};
	std::u8string prefix_{find_prefix()};
	unsigned major_{};
	// first GCC with modules support
	static constexpr unsigned gcc_with_modules = 11;
};

static compiler_registrar<gcc> register_gcc{
    "GCC"sv,
    "defined(__GNUC__)"sv,
    "__GNUC__ __GNUC_MINOR__ __GNUC_PATCHLEVEL__"sv,
};

std::vector<templated_string> gcc::commands_for(rule_type type) {
	switch (type) {
		case rule_type::COMPILE:
			return {
			    // g++ $DEFINES $FLAGS -fmodules-ts -c $in -o $out
			    {as_str(compiler_.generic_u8string()), " "s, var::DEFINES, " "s,
			     var::CFLAGS, " "s, var::CXXFLAGS, " -fmodules-ts -c "s,
			     var::INPUT, " -o "s, var::OUTPUT},
			};
		case rule_type::LINK_EXECUTABLE:
			return {
			    // g++ $in -o $out
			    {as_str(compiler_.generic_u8string()), " "s, var::INPUT,
			     " -o "s, var::OUTPUT},
			};
		case rule_type::ARCHIVE:
			return {
			    // rm -rf $out
			    {"rm  -rf "s, var::OUTPUT},
			    // ar qc $out $in
			    {toolname("ar"sv), " qc "s, var::OUTPUT, " "s, var::LINK_FLAGS,
			     " "s, var::INPUT},
			    // ranlib $out
			    {toolname("ranlib"sv), " "s, var::OUTPUT},
			};
		default:
			break;
	}
	return {};
}

void gcc::mapout(build_info const& build, generator& gen) {
	std::vector<target> targets;

	auto ids = register_projects(build, gen);

	unsigned long long rules_needed{};
	for (auto const& [prj, info] : build.projects) {
		auto const setup_id = get_setup_id(prj.name, ids);

		for (auto const& filename : info.sources) {
			auto const srcfile = (info.subdir / filename).generic_u8string();
			auto const objfile = filename + u8".o";

			auto const mods_it = build.imports.find(srcfile);
			auto const iface_it = build.exports.find(srcfile);

			auto const has_modules = mods_it != build.imports.end();
			auto const is_interface = iface_it != build.exports.end();

			{
				target source{
				    {},
				    file_ref{setup_id, filename, file_ref::input},
				};
				targets.push_back(std::move(source));
			}

			{
				rules_needed |= bit(rule_type::COMPILE);
				target object{rule_type::COMPILE, file_ref{setup_id, objfile}};
				if (is_interface) {
					object.outputs.impl.push_back(
					    mod_ref{iface_it->second, as_cmi(iface_it->second)});
					object.edge = iface_it->second.toString();
				}
				object.inputs.expl.push_back(
				    file_ref{setup_id, filename, file_ref::input});

				if (has_modules) {
					for (auto const& import : mods_it->second) {
						object.inputs.order.push_back(
						    mod_ref{import, as_cmi(import)});
					}
				}

				targets.push_back(std::move(object));
			}
		}

		{
			auto library = create_project_target(build, prj, info, ids);
			if (std::holds_alternative<rule_type>(library.rule)) {
				rules_needed |= bit(std::get<rule_type>(library.rule));
			}
			targets.push_back(std::move(library));
		}
	}

	add_rules(rules_needed, gen);
	gen.set_targets(std::move(targets));
}
