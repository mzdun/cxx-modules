#include "env/include_locator.hh"
#include <base/utils.hh>
#include <iostream>
#include <process.hpp>

namespace env {
	void include_locator::from_env(char const* name) {
#ifdef _WIN32
		auto const PATHSEP = ';';
#else
		auto const PATHSEP = ':';
#endif
		auto const env = std::getenv(name);
		if (!env) return;
		auto const dirs = split_s(PATHSEP, env);
		dirs_.clear();
		dirs_.reserve(dirs.size());
		std::copy(dirs.begin(), dirs.end(), std::back_inserter(dirs_));
	}

	fs::path include_locator::find_include(fs::path const& source_path,
	                                       std::u8string_view include) {
		if (include.size() < 3) return {};
		auto sys = include.front() == '<';
		include = include.substr(1, include.size() - 2);
		if (sys) return find_sys_include(include);
		return find_local_include(source_path, include);
	}

	std::vector<std::string> include_locator::resolve(
	    env::paths const& paths,
	    env::command const& in_cmd) {
		auto cmd = in_cmd.command_line_split();

		std::vector<std::string> result{};
		result.reserve(
		    1u + static_cast<size_t>(std::count_if(
		             cmd.args.begin(), cmd.args.end(), [](auto const& arg) {
			             return std::holds_alternative<std::string>(arg);
		             })));

		result.push_back(as_str(paths.which(cmd.tool)));
		for (auto& arg : cmd.args) {
			if (!std::holds_alternative<std::string>(arg)) continue;
			result.push_back(std::move(std::get<std::string>(arg)));
		}

		return result;
	}

	fs::path include_locator::find_sys_include(std::u8string_view filename) {
		parse_if_needed();

		for (auto const& dir : dirs_) {
			auto candidate = dir / filename;
			std::error_code ec{};
			auto status = fs::status(candidate, ec);
			if (!ec && fs::is_regular_file(status)) return candidate;
		}

		return {};
	}

	fs::path include_locator::find_local_include(fs::path const& source_path,
	                                             std::u8string_view filename) {
		auto candidate = source_path.parent_path() / filename;
		std::error_code ec{};
		auto status = fs::status(candidate, ec);
		if (!ec && fs::is_regular_file(status)) return candidate;
		return find_sys_include(filename);
	}

	void include_locator::parse_if_needed() {
		if (parsed_) return;
		parsed_ = true;
		dirs_.clear();

		std::string text{};
		auto const stream_is_out = use_stdout_;
		TinyProcessLib::Process preproc{
		    filter_,
		    "",
		    [&, stream_is_out](const char* bytes, size_t n) {
			    if (stream_is_out) text.append(bytes, n);
		    },
		    [&, stream_is_out](const char* bytes, size_t n) {
			    if (!stream_is_out) text.append(bytes, n);
		    },
		    true,
		};
		preproc.close_stdin();

		if (preproc.get_exit_status() != 0) {
			std::copy(filter_.begin(), filter_.end(),
			          std::ostream_iterator<std::string>{std::cerr, " "});
			std::cerr << '\n'
			          << "c++modules: error: command returned "
			          << preproc.get_exit_status() << '\n';
			return;
		}

		std::istringstream out{text};
		std::string line{};

		auto inlist = false;
		while (std::getline(out, line)) {
			if (line == filter_stop_) return;
			if (inlist) {
				dirs_.push_back(fs::path{strip_s(line)}
				                    .lexically_normal()
				                    .generic_u8string());
				continue;
			}
			if (line == filter_start_) inlist = true;
		}

		dirs_.clear();
	}
}  // namespace env
