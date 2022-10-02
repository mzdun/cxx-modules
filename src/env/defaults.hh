#pragma once

#include <filesystem>
#include <string_view>

namespace env {
	struct affix {
		using path = std::filesystem::path;
		std::u8string_view prefix{};
		std::u8string_view suffix{};
		path modify(path const&) const;
	};

	struct modifiers {
		affix executable{};
		affix static_library{};
		affix shared_library{};
		affix object{};
	};

	modifiers const& path_mods() noexcept;
}  // namespace env