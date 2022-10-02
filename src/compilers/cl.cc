#include <base/compiler.hh>
#include <env/binary_interface.hh>
#include <env/path.hh>
#include <env/defaults.hh>
#include <deque>

using namespace std::literals;

struct cl : compiler {
	cl(std::u8string_view, std::string_view, std::string_view) {}
	void mapout(struct build_info const&, generator&) override;
	std::vector<templated_string> commands_for(rule_type) override;
};

static compiler_registrar<cl> register_cl{
    "MSVC"sv,
    "defined(_MSC_VER)"sv,
    "_MSC_VER"sv,
};

void cl::mapout(build_info const& build, generator& gen) {
	std::vector<target> targets;
	env::binary_interface bin{true, true, "bmi.cache", ".ifc"};
	env::include_locator includes{{}, false, {}, {}, {}};
	includes.from_env("INCLUDE");

	auto const& mods = env::path_mods();
	auto ids = register_projects(build, gen);

	rule_types rules_needed{};
	for (auto const& [prj, info] : build.projects) {
		auto const setup_id = get_setup_id(prj.name, ids);

		for (auto const& filename : info.sources) {
			auto const srcfile = (info.subdir / filename).generic_u8string();
			auto const objfile =
			    mods.object.modify(filename).generic_u8string();

			auto const mods_it = build.imports.find(srcfile);
			auto const iface_it = build.exports.find(srcfile);

			auto const has_modules = mods_it != build.imports.end();
			auto const is_interface = iface_it != build.exports.end();

			{
				target source{
				    {},
				    file_ref{setup_id, filename, file_ref::input},
				};

				if (is_interface) {
					source.edge = iface_it->second.toString();
				}

				targets.push_back(std::move(source));
			}

			if (is_interface) {
				rules_needed.set(rule_type::EMIT_BMI);
				target bmi{rule_type::EMIT_BMI,
				           mod_ref{iface_it->second,
				                   bin.as_interface(iface_it->second)}};
				bmi.inputs.expl.push_back(
				    file_ref{setup_id, filename, file_ref::input});

				auto it = build.modules.find(iface_it->second);
				if (it != build.modules.end()) {
					auto& mod = it->second;
					for (auto const& req : mod.req) {
						auto art = bin.from_module(includes, srcfile, req);
						if (art) bmi.inputs.order.push_back(std::move(*art));
					}
				}
				targets.push_back(std::move(bmi));
			}

			{
				rules_needed.set(rule_type::COMPILE);
				target object{rule_type::COMPILE, file_ref{setup_id, objfile}};
				if (is_interface) {
					object.outputs.impl.push_back(mod_ref{
					    iface_it->second, bin.as_interface(iface_it->second)});
					object.edge = iface_it->second.toString();
				}
				object.inputs.expl.push_back(
				    file_ref{setup_id, filename, file_ref::input});

				if (has_modules) {
					for (auto const& import : mods_it->second) {
						auto art = bin.from_module(includes, srcfile, import);
						if (art) object.inputs.order.push_back(std::move(*art));
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
		{
			target library{
			    {},
			    file_ref{
			        setup_id,
			        prj.name,
			        file_ref::linked,
			    },
			};

			switch (prj.type) {
				case project::executable:
					library.rule = "VS-EXE";
					break;
				case project::static_lib:
					library.rule = "VS-LIB";
					break;
				case project::shared_lib:
					library.rule = "VS-DLL";
					break;
				case project::module_lib:
					library.rule = "VS-DLL";
					break;
			}

			for (auto const& filename : info.sources) {
				library.inputs.expl.push_back(
				    file_ref{setup_id, filename, file_ref::input});
			}

			if (prj.type != project::static_lib) {
				std::deque<project> stack{info.links.begin(), info.links.end()};
				std::set<project> seen{prj};

				while (!stack.empty()) {
					auto next = stack.front();
					stack.pop_front();

					auto [_, first] = seen.insert(next);
					if (!first) break;

					auto const next_id = get_setup_id(next.name, ids);
					library.inputs.impl.push_back(
					    file_ref{next_id, next.name, file_ref::linked});

					auto it = build.projects.find(next);
					if (it == build.projects.end()) continue;
					stack.insert(stack.end(), it->second.links.begin(),
					             it->second.links.end());
				}
			}
			targets.push_back(std::move(library));
		}
	}

	bin.add_targets(targets, rules_needed);

	add_rules(rules_needed, gen);


	gen.set_targets(std::move(targets));
}

std::vector<templated_string> cl::commands_for(rule_type) {
	return {};
}
