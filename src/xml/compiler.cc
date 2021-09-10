#include "compiler.hh"
#include "env/path.hh"
#include "generator.hh"
#include "process.hpp"
#include "types.hh"
#include "utils.hh"

using namespace std::literals;

namespace xml {
	std::vector<templated_string> compiler::commands_for(rule_type type) {
		return commands_.get(type);
	}

	void compiler::mapout(build_info const& build, generator& gen) {
		std::vector<target> targets;

		auto ids = register_projects(build, gen);

		auto const standalone_bmi = bin_.standalone_interface();

		rule_types rules_needed{};
		for (auto const& [prj, info] : build.projects) {
			auto const setup_id = get_setup_id(prj.name, ids);

			for (auto const& filename : info.sources) {
				auto const srcfile =
				    (info.subdir / filename).generic_u8string();
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

				if (standalone_bmi && is_interface) {
					rules_needed.set(rule_type::EMIT_BMI);
					target bmi{rule_type::EMIT_BMI,
					           mod_ref{iface_it->second,
					                   bin_.as_interface(iface_it->second)}};
					bmi.inputs.expl.push_back(
					    file_ref{setup_id, filename, file_ref::input});

					auto it = build.modules.find(iface_it->second);
					if (it != build.modules.end()) {
						auto& mod = it->second;
						for (auto const& req : mod.req) {
							auto art =
							    bin_.from_module(includes_, srcfile, req);
							if (art)
								bmi.inputs.order.push_back(std::move(*art));
						}
					}
					targets.push_back(std::move(bmi));
				}

				{
					rules_needed.set(rule_type::COMPILE);
					target object{rule_type::COMPILE,
					              file_ref{setup_id, objfile}};
					if (!standalone_bmi && is_interface) {
						object.outputs.impl.push_back(
						    mod_ref{iface_it->second,
						            bin_.as_interface(iface_it->second)});
						object.edge = iface_it->second.toString();
					}
					object.inputs.expl.push_back(
					    file_ref{setup_id, filename, file_ref::input});

					if (has_modules) {
						for (auto const& import : mods_it->second) {
							auto art =
							    bin_.from_module(includes_, srcfile, import);
							if (art)
								object.inputs.order.push_back(std::move(*art));
						}
					}

					targets.push_back(std::move(object));
				}
			}

			{
				auto library = create_project_target(build, prj, info, ids);
				if (std::holds_alternative<rule_type>(library.rule)) {
					rules_needed.set(std::get<rule_type>(library.rule));
				}
				targets.push_back(std::move(library));
			}
		}

		bin_.add_targets(targets, rules_needed);

		add_rules(rules_needed, gen);
		gen.set_targets(std::move(targets));
	}
}  // namespace xml
