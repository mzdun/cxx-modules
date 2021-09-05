#include "compiler.hh"
#include <deque>
#include <iostream>
#include <iterator>
#include <process.hpp>
#include "types.hh"
#include "utils.hh"

namespace {
	using namespace std::literals;

	std::vector<std::unique_ptr<compiler_factory>>& factories() {
		static std::vector<std::unique_ptr<compiler_factory>> impl{};
		return impl;
	}

	constexpr auto PATHSEP = ':';

	std::vector<fs::path> getPATH() {
		auto PATH = std::getenv("PATH");
		if (!PATH || !*PATH) return {};

		std::vector<fs::path> result{};

		std::string_view paths{PATH};
		size_t chunks = 1;
		for (auto c : paths) {
			if (c == PATHSEP) ++chunks;
		}
		result.reserve(chunks);

		size_t prev = 0;
		size_t current = 0;

		for (auto c : paths) {
			++current;
			if (c != PATHSEP) continue;
			auto curr = paths.substr(prev, current - 1 - prev);
			prev = current;
			if (!curr.empty()) result.emplace_back(curr);
		}

		return result;
	}

	fs::path fullpath(fs::path const& prog, bool real_path) {
		if (prog.native().find(fs::path::preferred_separator) !=
		        std::string::npos ||
		    prog.native().find('/') != std::string::npos)
			return prog;

		auto path = getPATH();
		for (auto const& dir : path) {
			auto candidate = dir / prog;
			std::error_code ec{};
			if (real_path) {
				auto status = fs::symlink_status(candidate, ec);
				if (ec ||
				    (!fs::is_regular_file(status) && !fs::is_symlink(status)))
					continue;
				while (fs::is_symlink(status)) {
					auto link = fs::read_symlink(candidate);
					if (!link.is_absolute()) {
						link =
						    (candidate.parent_path() / link).lexically_normal();
					}
					status = fs::symlink_status(link, ec);
					if (ec || (!fs::is_regular_file(status) &&
					           !fs::is_symlink(status)))
						break;
					candidate = link;
				}
			} else {
				auto status = fs::status(candidate, ec);
				if (ec || !fs::is_regular_file(status)) continue;
			}
			return candidate;
		}

		return prog;
	}

	fs::path compiler_executable() {
		auto CXX = std::getenv("CXX");
		if (CXX && *CXX) return CXX;
		return "c++";
	}

	void write(TinyProcessLib::Process& proc, std::string_view view) {
		proc.write(view.data(), view.size());
	}

	void write(TinyProcessLib::Process& proc, char sep) { proc.write(&sep, 1); }

	std::pair<std::string, std::string> compiler_type(fs::path const& cxx) {
		std::vector<std::string> args{cxx.generic_string(), "-E", "-o-",
		                              "-xc++", "-"};
		std::string text;
		TinyProcessLib::Process preproc{
		    args,
		    "",
		    [&](const char* bytes, size_t n) { text.append(bytes, n); },
		    [](const char* bytes, size_t n) {
			    std::cerr << std::string_view{bytes, n};
		    },
		    true,
		};
		bool printed{false};
		static constexpr auto hash_if = "#if"sv;
		static constexpr auto hash_ifelse = "#elif"sv;
		auto control = hash_if;
		for (auto const& factory : factories()) {
			auto const supplier = factory->get_compiler_id();
			write(preproc, control);
			write(preproc, ' ');
			write(preproc, supplier.if_macro);
			write(preproc, '\n');
			write(preproc, supplier.id);
			write(preproc, ' ');
			write(preproc, supplier.version_macros);
			write(preproc, '\n');
			control = hash_ifelse;
			printed = true;
		}
		if (printed) write(preproc, "#endif\n"sv);

		preproc.close_stdin();
		auto const exit_status = preproc.get_exit_status();
		if (exit_status) text.clear();
		auto lines = split_s('\n', text);
		for (auto& line : lines) {
			line = strip_s(split_s('#', line)[0]);
		}
		auto it = std::remove_if(
		    lines.begin(), lines.end(),
		    [](std::string const& line) { return line.empty(); });
		lines.erase(it, lines.end());
		text.clear();
		size_t length = 0;
		for (auto const& line : lines)
			length += line.size() + 1;
		text.reserve(length);

		for (auto const& line : lines) {
			text.append(line);
			text.push_back('\n');
		}

		text = strip_s(text);

		auto new_stop = text.size();
		decltype(new_stop) new_start = 0;

		while (new_start < new_stop &&
		       !std::isspace(static_cast<unsigned char>(text[new_start])))
			++new_start;

		auto type = text.substr(0, new_start);
		text = lstrip_s(text.substr(new_start));

		return {std::move(type), std::move(text)};
	}
}  // namespace

compiler::~compiler() = default;
compiler_factory::~compiler_factory() = default;

void compiler::mapout(build_info const&, struct generator&) {
	// noop
}

std::vector<templated_string> compiler::commands_for(rule_type) {
	return {};
}

std::map<std::u8string, size_t> compiler::register_projects(
    build_info const& build,
    generator& gen) {
	std::map<std::u8string, size_t> setup_ids{};

	std::vector<project_setup> setups{};
	setups.reserve(build.projects.size());

	for (auto const& [prj, info] : build.projects) {
		auto const id = setups.size();
		setups.push_back(
		    {prj.name, prj.name + u8".dir", info.subdir.generic_u8string()});
		setup_ids[prj.name] = id;
	}

	gen.set_setups(std::move(setups));
	return setup_ids;
}

target compiler::create_project_target(
    build_info const& build,
    project const& prj,
    project_info const& info,
    std::map<std::u8string, size_t> const& ids) {
	auto const setup_id = get_setup_id(prj.name, ids);

	target library{
	    {},
	    file_ref{
	        setup_id,
	        prj.filename(),
	        file_ref::linked,
	    },
	};

	switch (prj.type) {
		case project::executable:
			library.rule = rule_type::LINK_EXECUTABLE;
			break;
		case project::static_lib:
			library.rule = rule_type::ARCHIVE;
			break;
		case project::shared_lib:
			library.rule = rule_type::LINK_SO;
			break;
		case project::module_lib:
			library.rule = rule_type::LINK_MOD;
			break;
	}

	for (auto const& filename : info.sources) {
		auto const objfile = filename + u8".o";

		library.inputs.expl.push_back(file_ref{setup_id, objfile});
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
			library.inputs.expl.push_back(
			    file_ref{next_id, next.filename(), file_ref::linked});

			auto it = build.projects.find(next);
			if (it == build.projects.end()) continue;
			stack.insert(stack.end(), it->second.links.begin(),
			             it->second.links.end());
		}
	}

	return library;
}

void compiler::add_rules(unsigned long long const rules_needed,
                         generator& gen) {
	static constexpr rule_type rules[] = {
#define ENUM(NAME) rule_type::NAME,
	    RULE(ENUM)
#undef ENUM
	};

	size_t length{};
	for (auto rule : rules) {
		auto const test = bit(rule);
		if ((rules_needed & test) == 0) continue;
		++length;
	}

	std::vector<rule> results;
	results.reserve(length);

	for (auto rule : rules) {
		auto const test = bit(rule);
		if ((rules_needed & test) == 0) continue;
		results.push_back({rule, commands_for(rule)});
	}
	gen.set_rules(std::move(results));
}

fs::path compiler::where(fs::path const& toolname, bool real_paths) {
	return fullpath(toolname, real_paths);
}

size_t compiler_info::register_impl(std::unique_ptr<compiler_factory>&& impl) {
	factories().push_back(std::move(impl));
	return factories().size();
}

compiler_info compiler_info::from_environment() {
	auto const var = compiler_executable();
	/*auto const is_clang = [&] {
	    auto const fname = var.filename().generic_u8string();
	    return fname == u8"clang++"sv || fname.starts_with(u8"clang++-"sv) ||
	           fname.starts_with(u8"clang++."sv);
	}();*/
	compiler_info result{fullpath(var, false /*!is_clang*/), var.string(), {}};
	result.id = compiler_type(result.exec);
	for (auto const& impl : factories()) {
		if (impl->get_compiler_id().id != result.id.first) continue;
		result.factory = impl.get();
		break;
	}
	return result;
}

std::optional<std::string> compiler_info::preproc(
    fs::path const& source) const {
	std::vector<std::string> args{exec.generic_string(), "-E", "-o-", "-xc++",
	                              source.generic_string()};
	std::optional<std::string> text{std::string{}};
	std::string error_out{};
	TinyProcessLib::Process preproc{
	    args, "", [&](const char* bytes, size_t n) { text->append(bytes, n); },
	    [&](const char* bytes, size_t n) { error_out.append(bytes, n); }};
	if (preproc.get_exit_status() != 0) {
		text = std::nullopt;
		std::copy(args.begin(), args.end(),
		          std::ostream_iterator<std::string>{std::cerr, " "});
		std::cerr << '\n';
	}
	if (!error_out.empty()) {
		std::cerr << error_out;
	}
	return text;
}
