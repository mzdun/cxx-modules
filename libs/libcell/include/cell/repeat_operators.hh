#pragma once
#include "parser.hh"

namespace cell {
	template <class Subject>
	struct zero_or_more : unary_parser<zero_or_more, Subject> {
		constexpr zero_or_more(const Subject& subject)
			: unary_parser<cell::zero_or_more, Subject>(subject)
		{
		}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			while (this->subject.parse(first, last, ctx)) {}
			return true;
		}
	};

	template <class Subject>
	constexpr inline zero_or_more<as_parser_t<Subject>> operator*(Subject const& subject) noexcept {
		return { as_parser(subject) };
	}

	template <class Subject>
	struct one_or_more : unary_parser<one_or_more, Subject> {
		constexpr one_or_more(const Subject& subject)
			: unary_parser<cell::one_or_more, Subject>(subject)
		{
		}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			if (!this->subject.parse(first, last, ctx))
				return false;

			while (this->subject.parse(first, last, ctx)) {}
			return true;
		}
	};

	template <class Subject>
	constexpr inline one_or_more<as_parser_t<Subject>> operator+(Subject const& subject) noexcept {
		return { as_parser(subject) };
	}

	template <class Subject>
	struct zero_or_one : unary_parser<zero_or_one, Subject> {
		constexpr zero_or_one(const Subject& subject)
			: unary_parser<cell::zero_or_one, Subject>(subject)
		{
		}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			this->subject.parse(first, last, ctx);
			return true;
		}
	};

	template <class Subject>
	constexpr inline zero_or_one<as_parser_t<Subject>> operator-(Subject const& subject) noexcept {
		return { as_parser(subject) };
	}

	template <class Subject>
	struct exactly : unary_parser<exactly, Subject> {
		constexpr exactly(const Subject& subject, std::size_t count)
			: unary_parser<cell::exactly, Subject>(subject)
			, count(count)
		{
		}

		std::size_t count;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			for (std::size_t n = 0; n < count; ++n) {
				if (!this->subject.parse(first, last, ctx))
					return false;
			}
			return true;
		}
	};

	template <class Subject>
	struct at_least : unary_parser<at_least, Subject> {
		constexpr at_least(const Subject& subject, std::size_t count)
			: unary_parser<cell::at_least, Subject>(subject)
			, count(count)
		{
		}

		std::size_t count;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			for (std::size_t n = 0; n < count; ++n) {
				if (!this->subject.parse(first, last, ctx))
					return false;
			}

			while (this->subject.parse(first, last, ctx)) {}
			return true;
		}
	};

	template <template <class> class Handler>
	struct single_repeat_generator {
		constexpr single_repeat_generator(std::size_t count) : count(count) {}
		std::size_t count;

		template <typename Subject>
		constexpr Handler<Subject> operator()(const Subject& subject) {
			return Handler<Subject>(subject, count);
		}
	};

	template <class Subject>
	struct between : unary_parser<between, Subject> {
		constexpr between(const Subject& subject, std::size_t lower, std::size_t upper)
			: unary_parser<cell::between, Subject>(subject)
			, lower(lower)
			, upper(upper)
		{
		}

		std::size_t lower;
		std::size_t upper;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			for (std::size_t n = 0; n < lower; ++n) {
				if (!this->subject.parse(first, last, ctx))
					return false;
			}

			for (std::size_t n = lower; n < upper; ++n) {
				if (!this->subject.parse(first, last, ctx))
					break;
			}
			return true;
		}
	};

	struct between_generator {
		constexpr between_generator(std::size_t lower, std::size_t upper)
			: lower(lower), upper(upper)
		{}

		std::size_t lower;
		std::size_t upper;

		template <typename Subject>
		constexpr between<as_parser_t<Subject>> operator()(const Subject& subject) {
			return { as_parser(subject), lower, upper };
		}
	};

	class infinity_tag {};
	constexpr auto inf = infinity_tag{};

	struct repeat_generator {
		constexpr repeat_generator() = default;
		constexpr single_repeat_generator<exactly> operator()(std::size_t count) const {
			return count;
		}
		constexpr single_repeat_generator<at_least> operator()(std::size_t count, infinity_tag) const {
			return count;
		}
		constexpr between_generator operator()(std::size_t lower, std::size_t upper) const {
			return { lower, upper };
		}
	};
	constexpr auto repeat = repeat_generator{};
}
