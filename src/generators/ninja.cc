#include "ninja.hh"
#include <fstream>
#include <set>

#include "../utils.hh"

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
				    return {};
			    else if constexpr (std::is_same_v<decltype(name),
			                                      std::string const&>)
				    return name;
			    else {
				    switch (name) {
					    case rule_type::MKDIR:
						    return {};  // ninja creates directories by it's own
					    case rule_type::COMPILE:
						    return "cc"sv;
					    case rule_type::EMIT_BMI:
						    return "bmi"sv;
					    case rule_type::ARCHIVE:
						    return "ar"sv;
					    case rule_type::LINK_SO:
						    return "link-so"sv;
					    case rule_type::LINK_MOD:
						    return "link-macos-so"sv;
					    case rule_type::LINK_EXECUTABLE:
						    return "link-exe"sv;
				    }
				    return {};
			    }
		    },
		    name);
	}

	bool ignorable(rule_name const& name) {
		return std::visit(
		    [](auto const& name) -> bool {
			    if constexpr (std::is_same_v<decltype(name),
			                                 rule_type const&>) {
				    switch (name) {
					    case rule_type::MKDIR:
						    return true;
					    case rule_type::COMPILE:
					    case rule_type::EMIT_BMI:
					    case rule_type::ARCHIVE:
					    case rule_type::LINK_SO:
					    case rule_type::LINK_MOD:
					    case rule_type::LINK_EXECUTABLE:
						    break;
				    }
			    }
			    return false;
		    },
		    name);
	}
}  // namespace

void ninja::generate(std::filesystem::path const& back_to_sources,
                     std::filesystem::path const& builddir) {
	std::ofstream build_ninja{builddir / u8"build.ninja"sv};

	auto visitor = [&](auto const& arg) {
		if constexpr (std::is_same_v<decltype(arg), std::string const&>) {
			build_ninja << arg;
		} else {
			build_ninja << varname(arg);
		}
	};

	build_ninja << "CXXFLAGS = -std=c++20 -O0 -g\n"
	               "\n";

	for (auto const& rule : rules_) {
		auto const name = name2sv(rule.name);
		if (name.empty()) continue;

		build_ninja << "rule " << name << '\n';

		build_ninja << "    command = ";
		bool first_command = true;
		for (auto const& cmd : rule.commands) {
			if (first_command)
				first_command = false;
			else
				build_ninja << " && ";
			for (auto const& arg : cmd)
				std::visit(visitor, arg);
		}
		build_ninja << '\n';

		auto msg = rule.message;
		if (msg.empty() && std::holds_alternative<rule_type>(rule.name))
			msg = rule::default_message(std::get<rule_type>(rule.name));
		if (!msg.empty()) {
			build_ninja << "    description = ";
			for (auto const& chunk : msg) {
				std::visit(visitor, chunk);
			}
			build_ninja << '\n';
		}

		build_ninja << '\n';
	}

	std::set<artifact> ignored;
	for (auto const& target : targets_) {
		if (ignorable(target.rule)) {
			ignored.insert(target.main_output);
			ignored.insert(target.outputs.expl.begin(),
			               target.outputs.expl.end());
			ignored.insert(target.outputs.impl.begin(),
			               target.outputs.impl.end());
			ignored.insert(target.outputs.order.begin(),
			               target.outputs.order.end());
		}
	}

	for (auto const& target : targets_) {
		auto const name = name2sv(target.rule);
		if (name.empty()) continue;

		build_ninja << "build "
		            << as_sv(filename(back_to_sources, target.main_output));
		for (auto const& out : target.outputs.expl)
			build_ninja << ' ' << as_sv(filename(back_to_sources, out));
		if (!target.outputs.impl.empty() || !target.outputs.order.empty())
			build_ninja << " |";
		for (auto const& out : target.outputs.impl)
			build_ninja << ' ' << as_sv(filename(back_to_sources, out));
		for (auto const& out : target.outputs.order)
			build_ninja << ' ' << as_sv(filename(back_to_sources, out));

		build_ninja << ": " << name;

		for (auto const& in : target.inputs.expl) {
			if (ignored.count(in) != 0) continue;
			build_ninja << ' ' << as_sv(filename(back_to_sources, in));
		}
		bool first = true;
		for (auto const& in : target.inputs.impl) {
			if (ignored.count(in) != 0) continue;
			if (first) {
				first = false;
				build_ninja << " |";
			}
			build_ninja << ' ' << as_sv(filename(back_to_sources, in));
		}
		first = true;
		for (auto const& in : target.inputs.order) {
			if (ignored.count(in) != 0) continue;
			if (first) {
				first = false;
				build_ninja << " ||";
			}
			build_ninja << ' ' << as_sv(filename(back_to_sources, in));
		}
		build_ninja << '\n';
	}
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
	}
	return file.path;
}

std::u8string ninja::filename(std::filesystem::path const& back_to_sources,
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