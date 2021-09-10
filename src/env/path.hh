#pragma once

#include <base/generator.hh>
#include <fs/file.hh>
#include <string>

namespace env {
	struct tool {
		bool is_cxx{true};
		std::u8string name{};
	};

	struct command {
		env::tool tool{};
		templated_string args;

		command command_line_split() const;
	};

	struct paths {
		fs::path cxx;
		fs::path root;
		std::u8string triple;
		std::u8string suffix;
		bool is_gcc{false};

		std::u8string which(tool const& tool) const;

		struct parser {
			parser(fs::path const& exe,
			       std::u8string const& tool,
			       unsigned major)
			    : exe{exe}, tool{tool}, major{major} {}

			paths find() &&;

		private:
			std::tuple<bool, fs::path, std::u8string, std::u8string>
			break_triple(fs::path const& candidate);
			std::u8string mk_suffix() const {
				char buffer[200];
				std::snprintf(buffer, sizeof(buffer), "-%u", major);
				return reinterpret_cast<char8_t const*>(buffer);
			}

			fs::path exe;
			std::u8string tool;
			unsigned major;
			std::u8string major_str = mk_suffix();
			std::u8string vtool{tool + major_str};

			fs::path root{};
			std::u8string tripple{};
			std::u8string suffix{};
		};
	};

	fs::path which(fs::path const& tool);
};  // namespace env