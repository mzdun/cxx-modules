#pragma once
#include "parser.hh"

namespace cell {
	template <class ... Operands>
	struct alternative : nary_parser<alternative, Operands...> {
		using nary_parser<cell::alternative, Operands...>::nary_parser;

		template <class Right>
		constexpr inline alternative<Operands..., as_parser_t<Right>> operator|(Right const& rhs) const noexcept {
			return this->append(rhs);
		}

		template <typename Iterator, typename Context, std::size_t ... Index>
		bool indexed(Iterator& first, const Iterator& last, Context& ctx, std::index_sequence<Index...>) const {
			return (std::get<Index>(this->operands).parse(first, last, ctx) || ...);
		}
	};

	template <typename Type> struct is_alternative : std::false_type {};
	template <class ... Operands> struct is_alternative<alternative<Operands...>> : std::true_type {};

	template <class First, class Second>
	using disable_alternative = std::enable_if_t<!is_alternative<First>::value, alternative<First, Second>>;

	template <class First, class Second>
	constexpr inline disable_alternative<as_parser_t<First>, as_parser_t<Second>> operator|(First const& lhs, Second const& rhs) noexcept {
		return { as_parser(lhs), as_parser(rhs) };
	}

	template <class ... Operands>
	struct sequence : nary_parser<sequence, Operands...> {
		using nary_parser<cell::sequence, Operands...>::nary_parser;

		template <class Right>
		constexpr inline sequence<Operands..., as_parser_t<Right>> operator>>(Right const& rhs) const noexcept {
			return this->append(rhs);
		}

		template <typename Iterator, typename Context, std::size_t ... Index>
		bool indexed(Iterator& first, const Iterator& last, Context& ctx, std::index_sequence<Index...>) const {
			auto save = first;
			return (parse_one(std::get<Index>(this->operands), save, first, last, ctx) && ...);
		}

		template <typename Operand, typename Iterator, typename Context, std::size_t ... Index>
		bool parse_one(Operand const& op, Iterator const& save, Iterator& first, const Iterator& last, Context& ctx) const {
			if (!op.parse(first, last, ctx)) {
				first = save;
				return false;
			}
			return true;
		}
	};

	template <typename Type> struct is_sequence: std::false_type {};
	template <class ... Operands> struct is_sequence<sequence<Operands...>> : std::true_type {};

	template <class First, class Second>
	using disable_sequence = std::enable_if_t<!is_sequence<First>::value, sequence<First, Second>>;

	template <class First, class Second>
	constexpr inline disable_sequence<as_parser_t<First>, as_parser_t<Second>> operator>>(First const& lhs, Second const& rhs) noexcept {
		return { as_parser(lhs), as_parser(rhs) };
	}

	template <class Left, class Right>
	struct difference : binary_parser<difference, Left, Right> {
		using binary_parser<cell::difference, Left, Right>::binary_parser;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			auto save = first;
			if (this->right.parse(first, last, ctx)) {
				first = save;
				return false;
			}
			return this->left.parse(first, last, ctx);
		}
	};

	template <class Left, class Right>
	constexpr inline difference<as_parser_t<Left>, as_parser_t<Right>> operator-(Left const& lhs, Right const& rhs) noexcept {
		return { as_parser(lhs), as_parser(rhs) };
	}

	template <class Subject>
	struct not_parser : unary_parser<not_parser, Subject> {
		constexpr not_parser(const Subject& subject)
			: unary_parser<cell::not_parser, Subject>(subject)
		{
		}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			return !this->subject.parse(first, last, ctx);
		}
	};

	template <class Subject>
	constexpr inline not_parser<as_parser_t<Subject>> operator!(Subject const& subject) noexcept {
		return { as_parser(subject) };
	}
}