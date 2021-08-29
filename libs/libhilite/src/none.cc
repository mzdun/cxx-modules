#include "hilite/none.hh"
#include "hilite/hilite_impl.hh"

#include "cell/special.hh"
#include "cell/character.hh"
#include "cell/operators.hh"
#include "cell/repeat_operators.hh"

namespace hl::none {
	using namespace cell;

	RULE_EMIT(newline, token::newline)

	std::string_view token_to_string(unsigned tok) noexcept {
		using namespace std::literals;

#define STRINGIFY(x) case hl::x: return #x ## sv;
		switch (tok) {
			HILITE_TOKENS(STRINGIFY)
		default:
			break;
		}
#undef STRINGIFY

		return {};
	}

	void tokenize(const std::string_view& contents, callback& result) {
		auto begin = contents.begin();
		const auto end = contents.end();
		auto value = grammar_result{};

		parse_with_restart<grammar_value>(begin, end, *(eol[on_newline] | ch), empty, value);
		value.produce_lines(result, contents.size());
	}
}
