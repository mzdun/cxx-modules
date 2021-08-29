#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <vector>

#define HILITE_TOKENS(X) \
	X(whitespace) \
	X(newline) \
	X(line_comment) \
	X(block_comment) \
	X(identifier) \
	X(keyword) \
	X(module_name) \
	X(known_ident_1) \
	X(known_ident_2) \
	X(known_ident_3) \
	X(punctuator) \
	X(number) \
	X(character) \
	X(char_encoding) \
	X(char_delim) \
	X(char_udl) \
	X(string) \
	X(string_encoding) \
	X(string_delim) \
	X(string_udl) \
	X(escape_sequence) \
	X(raw_string) \
	X(meta) \
	X(meta_identifier)

namespace hl {
	enum token : unsigned {
#define LIST_TOKENS(x) x,
		HILITE_TOKENS(LIST_TOKENS)
#undef LIST_TOKENS
	};

	struct token_t {
		size_t start;
		size_t end;
		hl::token kind;
		bool operator < (const token_t& rhs) const {
			if (start == rhs.start) {
				if (end == rhs.end)
					return kind < rhs.kind;
				return end > rhs.end;
			}
			return start < rhs.start;
		}
		bool operator == (const token_t& rhs) const {
			return (start == rhs.start)
				&& (end == rhs.end)
				&& (kind == rhs.kind);
		}
	};
	using tokens = std::vector<token_t>;

	struct token_stack_t {
		size_t end;
		hl::token kind;
	};

	struct callback {
		callback();
		callback(const callback&) = delete;
		callback(callback&&);
		callback& operator=(const callback&) = delete;
		callback& operator=(callback&&);

		virtual ~callback();
		virtual void on_line(std::size_t start, std::size_t length, const tokens& highlights) = 0;
	};
}
