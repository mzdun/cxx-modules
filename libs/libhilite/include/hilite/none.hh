#pragma once
#include "hilite/hilite.hh"

namespace hl::none {
	std::string_view token_to_string(unsigned) noexcept;
	void tokenize(const std::string_view& contents, callback& result);
}
