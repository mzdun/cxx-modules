#include "base/compiler.hh"
#include <base/utils.hh>
#include <deque>
#include <env/path.hh>
#include <fstream>
#include <iostream>
#include <iterator>
#include <process.hpp>
#include "types.hh"

namespace {
	using namespace std::literals;

	compiler_info::category get_compiler_category_by_name(
	    fs::path const& exec) {
		auto const filename = exec.filename();
		if (filename == u8"cl.exe"sv || filename == u8"cl"sv)
			return compiler_info::vc;
		return compiler_info::gcc_like;
	}

	std::vector<std::unique_ptr<compiler_factory>>& factories() {
		static std::vector<std::unique_ptr<compiler_factory>> impl{};
		return impl;
	}

	fs::path compiler_executable() {
		auto CXX = std::getenv("CXX");
		if (CXX && *CXX) return CXX;
		return "c++";
	}

	std::optional<std::string> preproc_file(fs::path const& cxx,
	                                        fs::path const& source,
	                                        compiler_info::category cat) {
		auto const exec = cxx.generic_string();
		auto const srcfile = source.generic_string();
		auto const args =
		    cat == compiler_info::vc
		        ? std::vector<std::string>{exec, "/E", srcfile}
		        : std::vector<std::string>{exec, "-E", "-o-", "-xc++", srcfile};

		std::optional<std::string> text{std::string{}};
		std::string error_out{};

		TinyProcessLib::Process preproc{
		    args, "",
		    [&](const char* bytes, size_t n) { text->append(bytes, n); },
		    [&](const char* bytes, size_t n) { error_out.append(bytes, n); }};

		if (preproc.get_exit_status() != 0) {
			text = std::nullopt;
			std::copy(args.begin(), args.end(),
			          std::ostream_iterator<std::string>{std::cerr, " "});
			std::cerr << '\n'
			          << "c++modules: error: command returned "
			          << preproc.get_exit_status() << '\n';
		}

		if (!error_out.empty()) {
			std::cerr << error_out;
		}

		return text;
	}

	void write(TinyProcessLib::Process& proc, std::string_view view) {
		proc.write(view.data(), view.size());
	}

	void write(TinyProcessLib::Process& proc, char sep) { proc.write(&sep, 1); }

	static constexpr auto cxx_modules = u8"c++modules"sv;
	static constexpr auto ident_file = u8"ident.cpp"sv;

	bool write_ident_cpp(fs::path const& build_dir) {
		auto const dir = build_dir / cxx_modules;
		std::error_code ec{};
		fs::create_directories(dir, ec);
		if (ec) {
			std::cerr << "c++modules: cannot create "
			          << as_sv(dir.generic_u8string()) << ": " << ec.message()
			          << '\n';
			return false;
		}

		std::ofstream of{dir / ident_file};

		bool printed{false};
		static constexpr auto hash_if = "#if"sv;
		static constexpr auto hash_ifelse = "#elif"sv;
		auto control = hash_if;
		for (auto const& factory : factories()) {
			auto const supplier = factory->get_compiler_id();
			of << control << ' ' << supplier.if_macro << '\n';
			of << supplier.id << ' ' << supplier.version_macros << '\n';
			control = hash_ifelse;
			printed = true;
		}
		if (printed) of << "#endif\n"sv;
		return true;
	}

	std::string cleanup(std::optional<std::string>&& text) {
		if (!text) return {};
		auto lines = split_s('\n', *text);
		for (auto& line : lines) {
			line = strip_s(split_s('#', line)[0]);
		}
		auto it = std::remove_if(
		    lines.begin(), lines.end(),
		    [](std::string const& line) { return line.empty(); });
		lines.erase(it, lines.end());
		size_t length = 0;
		for (auto const& line : lines)
			length += line.size() + 1;

		std::string result{};
		result.reserve(length);

		for (auto const& line : lines) {
			result.append(line);
			result.push_back('\n');
		}

		return strip_s(result);
	}

	std::pair<std::string, std::string> compiler_type(
	    fs::path const& build_dir,
	    fs::path const& cxx,
	    compiler_info::category cat) {
		if (!write_ident_cpp(build_dir)) return {};

		auto text = cleanup(
		    preproc_file(cxx, build_dir / cxx_modules / ident_file, cat));

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

void compiler::add_rules(rule_types const rules_needed, generator& gen) {
	static constexpr rule_type rules[] = {
#define ENUM(NAME) rule_type::NAME,
	    RULE(ENUM)
#undef ENUM
	};

	size_t length{};
	for (auto rule : rules) {
		if (!rules_needed.has(rule)) continue;
		++length;
	}

	std::vector<rule> results;
	results.reserve(length);

	for (auto rule : rules) {
		if (!rules_needed.has(rule)) continue;
		results.push_back({rule, commands_for(rule)});
	}
	gen.set_rules(std::move(results));
}

size_t compiler_info::register_impl(std::unique_ptr<compiler_factory>&& impl) {
	factories().push_back(std::move(impl));
	return factories().size();
}

compiler_info compiler_info::from_environment(fs::path const& build_dir) {
	auto const var = compiler_executable();
	compiler_info result{env::which(var), var.string(), {}};
	result.cat = get_compiler_category_by_name(result.exec);
	result.id = compiler_type(build_dir, result.exec, result.cat);
	for (auto const& impl : factories()) {
		if (impl->get_compiler_id().id != result.id.first) continue;
		result.factory = impl.get();
		break;
	}
	return result;
}

std::optional<std::string> compiler_info::preproc(
    fs::path const& source) const {
	return preproc_file(exec, source, cat);
}
