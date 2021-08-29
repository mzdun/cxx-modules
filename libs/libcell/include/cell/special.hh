#pragma once

#include "parser.hh"

namespace cell {
	struct end_of_line : character_parser<end_of_line> {
		constexpr end_of_line() = default;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context&) const {
			auto copy = first;
			auto matched = false;
			if (copy != last && *copy == '\r') {
				++copy;
				matched = true;
			}
			if (copy != last && *copy == '\n') {
				++copy;
				matched = true;
			}
			if (matched)
				first = copy;
			return matched;
		}
	};

	constexpr auto eol = end_of_line{};

	struct end_of_file : parser<end_of_file> {
		constexpr end_of_file() = default;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context&) const {
			return first == last;
		}
	};

	constexpr auto eof = end_of_file{};

	struct epsilon : parser<epsilon> {
		constexpr epsilon() = default;

		template <typename Iterator, typename Context>
		bool parse(Iterator&, const Iterator&, Context&) const {
			return true;
		}
	};

	constexpr auto eps = epsilon{};

	struct nothing : parser<nothing> {
		constexpr nothing() = default;

		template <typename Iterator, typename Context>
		bool parse(Iterator&, const Iterator&, Context&) const {
			return false;
		}
	};

	constexpr auto empty = nothing{};
}
