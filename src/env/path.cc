#include "env/path.hh"
#include "compiler.hh"
#include "utils.hh"

using namespace std::literals;

namespace env {
	namespace {
#ifdef _WIN32
		constexpr auto PATHSEP = ';';
#else
		constexpr auto PATHSEP = ':';
#endif

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

		fs::path fullpath(fs::path const& prog) {
			if (prog.native().find(fs::path::preferred_separator) !=
			        std::string::npos ||
			    prog.native().find('/') != std::string::npos)
				return prog;

			auto path = getPATH();
			for (auto const& dir : path) {
				auto candidate = dir / prog;
				std::error_code ec{};
				auto status = fs::status(candidate, ec);
				if (ec || !fs::is_regular_file(status)) continue;
				return candidate;
			}

			return prog;
		}
	}  // namespace

	command command::command_line_split() const {
		command result{tool, {}};
		for (auto const& arg : args) {
			if (!std::holds_alternative<std::string>(arg)) {
				result.args.push_back(arg);
				continue;
			}

			auto const& str = std::get<std::string>(arg);
			auto const view = std::string_view{str};

			auto it = view.begin();
			auto end = view.end();

			while (it != end) {
				while (it != end &&
				       std::isspace(static_cast<unsigned char>(*it)))
					++it;
				if (it == end) continue;

				std::string new_arg{};
				new_arg.reserve(static_cast<size_t>(end - it));

				auto const quote = *it;
				bool quoted = quote == '\'' || quote == '"';
				if (quoted) ++it;

				bool escaped = false;
				for (; it != end; ++it) {
					auto const ch = *it;
					if (!escaped) {
						if (quoted && ch == quote) break;

						if (!quoted) {
							if (std::isspace(static_cast<unsigned char>(ch)))
								break;
							if (ch == '\'' || ch == '"') break;
						}

						if (ch == '\\' && quote == '"') {
							escaped = true;
							continue;
						}
						// "fall-through"
					}
					new_arg.push_back(ch);
				}

				if (!new_arg.empty() || quoted)
					result.args.push_back(std::move(new_arg));
			}
		}
		return result;
	}

	std::u8string paths::which(tool const& tool) const {
		if (tool.is_cxx) return cxx.generic_u8string();

		static constexpr auto gcc = u8"-gcc"sv;
		static constexpr auto dash = u8"-"sv;

		// 1. root / prefix-gcc-tool-suffix
		// 2. root / prefix-tool-suffix
		// 3. root / prefix-gcc-tool
		// 4. root / prefix-tool
		// 5. root / tool-suffix
		// 6. root / tool
		// 7. where tool

		if (!triple.empty()) {
			if (is_gcc) {
				if (!suffix.empty()) {
					auto const path =
					    root / u8concat(triple, gcc, dash, tool.name, suffix);

					std::error_code ec{};
					if (fs::exists(path, ec)) return path.generic_u8string();
				}

				auto const path = root / u8concat(triple, gcc, dash, tool.name);

				std::error_code ec{};
				if (fs::exists(path, ec)) return path.generic_u8string();
			}

			if (!suffix.empty()) {
				auto const path =
				    root / u8concat(triple, dash, tool.name, suffix);

				std::error_code ec{};
				if (fs::exists(path, ec)) return path.generic_u8string();
			}

			auto const path = root / u8concat(triple, dash, tool.name);

			std::error_code ec{};
			if (fs::exists(path, ec)) return path.generic_u8string();
		}

		if (!suffix.empty()) {
			auto const path = root / u8concat(tool.name, suffix);

			std::error_code ec{};
			if (fs::exists(path, ec)) return path.generic_u8string();
		}

		auto const path = root / tool.name;

		std::error_code ec{};
		if (exists(path, ec)) return path.generic_u8string();

		return fullpath(tool.name).generic_u8string();
	}

	paths paths::parser::find() && {
		auto const is_gcc = tool == u8"g++"sv;
		paths result{};

		std::error_code ec{};
		auto candidate = exe;
		auto status = symlink_status(candidate, ec);

		{
			auto const [valid, root, tripple, suffix] = break_triple(candidate);
			if (valid) {
				result = {exe, root, tripple, suffix, is_gcc};
				if (!tripple.empty()) return result;
			}

			if (ec || (!is_regular_file(status) && !is_symlink(status)))
				return result;
		}

		while (is_symlink(status)) {
			auto link = read_symlink(candidate);
			if (!link.is_absolute()) {
				link = (candidate.parent_path() / link).lexically_normal();
			}
			status = symlink_status(link, ec);
			if (ec || (!is_regular_file(status) && !is_symlink(status))) break;
			candidate = link;
			auto const [valid, root, tripple, suffix] = break_triple(candidate);
			if (valid) {
				result = {exe, root, tripple, suffix, is_gcc};
				if (!tripple.empty()) return result;
			}
		}

		return result;
	}

	std::tuple<bool, fs::path, std::u8string, std::u8string>
	paths::parser::break_triple(fs::path const& candidate) {
		auto const root = candidate.parent_path();

		auto const toolname_s = candidate.filename().generic_u8string();
		auto const toolname = std::u8string_view{toolname_s};

		if (toolname == tool) return {true, root, {}, {}};
		if (toolname == vtool) return {true, root, {}, major_str};

		auto pos = toolname.find('-'), last_dash = std::u8string_view::npos;
		size_t dashes = 0;

		while (pos != std::u8string_view::npos && dashes < 3) {
			++dashes;
			last_dash = pos;
			pos = toolname.find('-',
			                    pos + 2);  // at least one more char between
		}

		if (dashes == 3) {
			auto const triple = as_u8str(toolname.substr(0, last_dash));
			auto const tool_suffix = toolname.substr(last_dash + 1);
			if (tool_suffix == tool) return {true, root, triple, {}};
			if (tool_suffix == vtool) return {true, root, triple, major_str};
		}

		return {false, {}, {}, {}};
	}

	fs::path which(fs::path const& tool) { return fullpath(tool); }
}  // namespace env
