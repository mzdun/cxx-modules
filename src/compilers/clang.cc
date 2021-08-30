#include "../compiler.hh"
#include "../generator.hh"
#include "../types.hh"
#include "../utils.hh"

using namespace std::literals;

namespace {
	std::u8string as_cmi(mod_name const& name) {
		static constexpr auto cache = u8"bmi/"sv;
		static constexpr auto ext = u8".pcm"sv;

		std::u8string fname{};
		fname.reserve(cache.size() + name.module.size() +
		              (name.part.empty() ? 0 : 1 + name.part.size()) +
		              ext.size());
		fname.append(cache);
		fname.append(name.module);
		if (!name.part.empty()) {
			// clang 14.0.0: error: sorry, module partitions are not yet
			// supported
			fname.push_back('.');
			fname.append(name.part);
		}
		fname.append(ext);
		return fname;
	}

	inline unsigned bit(rule_type rule) {
		return 1u << static_cast<std::underlying_type_t<rule_type>>(rule);
	}
}  // namespace

struct clang : compiler {
	clang(std::u8string_view exec, std::string_view, std::string_view)
	    : compiler_{exec} {}

	void mapout(build_info const& build, generator& gen) override;
	std::vector<templated_string> commands_for(rule_type)
	override;

private:
	fs::path compiler_{};
};

static compiler_registrar<clang> register_clang{
    "Clang"sv,
    "defined(__clang__)"sv,
    "__clang_major__ __clang_minor__ __clang_patchlevel__"sv,
};

std::vector<templated_string> clang::commands_for(rule_type type) {
	switch (type) {
		case rule_type::EMIT_BMI:
			return {
			    // clang++ $in $DEFINES $FLAGS -o $out -c -Xclang
			    // -emit-module-interface
			    {as_str(compiler_.generic_u8string()), " "s, var::INPUT, " "s,
			     var::DEFINES, " "s, var::CFLAGS, " "s, var::CXXFLAGS, " -o "s,
			     var::OUTPUT,
			     " -fprebuilt-implicit-modules -fprebuilt-module-path=bmi -c"
			     " -Xclang -emit-module-interface"},
			};
		case rule_type::COMPILE:
			return {
			    // clang++ $in $DEFINES $FLAGS -o $out -c
			    {as_str(compiler_.generic_u8string()), " "s, var::INPUT, " "s,
			     var::DEFINES, " "s, var::CFLAGS, " "s, var::CXXFLAGS, " -o "s,
			     var::OUTPUT,
			     " -fprebuilt-implicit-modules -fprebuilt-module-path=bmi -c"},
			};
		case rule_type::LINK_EXECUTABLE:
			return {
			    // g++ $in -o $out
			    {as_str(compiler_.generic_u8string()), " "s, var::LINK_FLAGS,
			     " "s, var::INPUT, " -o "s, var::OUTPUT},
			};
		case rule_type::ARCHIVE:
			return {
			    // rm -rf $out
			    {"rm  -rf "s, var::OUTPUT},
			    // ar qc $out $in
			    {where("ar"sv, false).string(), " qc "s, var::OUTPUT, " "s,
			     var::LINK_FLAGS, " "s, var::INPUT},
			    // ranlib $out
			    {where("ranlib"sv, false).string(), " "s, var::OUTPUT},
			};
		default:
			break;
	}
	return {};
}

void clang::mapout(build_info const& build, generator& gen) {
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

			if (is_interface) {
				rules_needed |= bit(rule_type::EMIT_BMI);
				target bmi{rule_type::EMIT_BMI,
				           mod_ref{iface_it->second, as_cmi(iface_it->second)}};
				bmi.inputs.expl.push_back(
				    file_ref{setup_id, filename, file_ref::input});

				auto it = build.modules.find(iface_it->second);
				if (it != build.modules.end()) {
					auto& mod = it->second;
					for (auto const& req : mod.req)
						bmi.inputs.order.push_back(mod_ref{req, as_cmi(req)});
				}
				targets.push_back(std::move(bmi));
			}

			{
				rules_needed |= bit(rule_type::COMPILE);
				target object{rule_type::COMPILE, file_ref{setup_id, objfile}};
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