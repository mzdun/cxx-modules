#pragma once

#include <env/path.hh>

namespace env {
	// TODO: cmake projects have private/interface include dirs...
	struct include_locator {
		include_locator(env::paths const& paths,
		                bool use_stdout,
		                std::string const& filter_start,
		                std::string const& filter_stop,
		                env::command const& filter)
		    : use_stdout_{use_stdout}
		    , filter_start_{filter_start}
		    , filter_stop_{filter_stop}
		    , filter_{resolve(paths, filter)} {}

		void from_env(char const* name);

		fs::path find_include(fs::path const& source_path,
		                      std::u8string_view include);

	private:
		static std::vector<std::string> resolve(env::paths const& paths,
		                                        env::command const& in_cmd);

		fs::path find_sys_include(std::u8string_view filename);

		fs::path find_local_include(fs::path const& source_path,
		                            std::u8string_view filename);

		void parse_if_needed();

		bool parsed_{false};
		bool use_stdout_{true};
		std::string filter_start_{};
		std::string filter_stop_{};
		std::vector<std::string> filter_;
		std::vector<fs::path> dirs_;
	};
}  // namespace env
