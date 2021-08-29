#pragma once

#include <type_traits>
#include <tuple>
#include <utility> // for all descendant from nary_parser

namespace cell {
	struct basic_parser {
		constexpr basic_parser() = default;
	};

	template <class Subject, class Action>
	struct action;

	template <class Derived>
	struct parser : basic_parser {
		constexpr parser() = default;
		constexpr Derived const& derived() const
		{
			return *static_cast<Derived const*>(this);
		}

		template <class Action>
		constexpr action<Derived, Action> operator[](const Action& action) const {
			return { this->derived(), action };
		}
	};

	namespace ext {
		template <class P, class Enable = void> struct as_parser {};
		template <class Derived> struct as_parser<parser<Derived>> {
			using type = Derived;
		};
		template <class Derived>
		struct as_parser<
			Derived,
			std::enable_if_t<std::is_base_of_v<basic_parser, Derived>>
		> {
			using type = Derived;
		};
	}
	template <class P> using as_parser_t = typename ext::as_parser<P>::type;
	template <typename T>
	constexpr as_parser_t<T> as_parser(T const& val) {
		return as_parser_t<T>(val);
	}
	template <typename Derived>
	constexpr Derived const& as_parser(parser<Derived> const& par) {
		return par.derived();
	}

	template <template <class> class Derived, class Subject>
	struct unary_parser : parser<Derived<Subject>> {
		using subject_type = Subject;

		constexpr unary_parser(Subject const& subject) noexcept
			: subject{ subject }
		{}

		Subject subject;
	};

	template <template <class, class> class Derived, class Left, class Right>
	struct binary_parser : parser<Derived<Left, Right>> {
		using left_type = Left;
		using right_type = Right;

		constexpr binary_parser(Left const& left, Right const& right) noexcept
			: left{ left }
			, right{ right }
		{}

		Left left;
		Right right;
	};

	template <template <class...> class Derived, class ... Operands>
	struct nary_parser : parser<Derived<Operands...>> {

		constexpr nary_parser(Operands const& ... operands) noexcept
			: operands{ operands... }
		{}

		template <typename Right>
		constexpr Derived<Operands..., as_parser_t<Right>> append(Right const& rhs) const noexcept {
			return unpacked_append(rhs, std::make_index_sequence<sizeof...(Operands)>{});
		}

		template <class Right, std::size_t ... Index>
		constexpr inline Derived<Operands..., as_parser_t<Right>> unpacked_append(Right const& rhs, std::index_sequence<Index...>) const noexcept {
			return { std::get<Index>(operands)..., as_parser(rhs) };
		}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			return static_cast<Derived<Operands...> const *>(this)
				->indexed(first, last, ctx,
					std::make_index_sequence<sizeof...(Operands)>{}
			);
		}

		std::tuple<Operands...> operands;
	};

	template <class Derived>
	struct character_parser : parser<Derived> {
		constexpr character_parser() = default;

		template <typename Iterator, typename Context>
		void filter(Iterator& first, const Iterator& last, Context& ctx) const {
			auto prev = first;
			auto& skip = _filter(ctx);
			while (skip.parse(first, last, ctx) && prev != first)
				prev = first;
			first = prev;
		}
	};

	struct action_state {
		static bool enabled() noexcept {
			return get_enabled();
		}
		action_state() : previous_value(get_enabled()) {
			get_enabled() = false;
		}
		~action_state() {
			get_enabled() = previous_value;
		}
	private:
		bool previous_value;

		static bool& get_enabled() noexcept {
			thread_local bool value{ true };
			return value;
		}
	};

	template <class Subject, class Action>
	struct action : parser<action<Subject, Action>> {
		Subject subject;
		Action call;
		constexpr action(const Subject& subject, const Action& call)
			: subject(subject)
			, call(call)
		{
		}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			auto copy = first;
			if (this->subject.parse(first, last, ctx)) {
				if (action_state::enabled()) {
					_setrange(copy, first, ctx);
					this->call(ctx);
				}
				return true;
			}
			return false;
		}
	};

	template <class Subject>
	struct peek_parser : unary_parser<peek_parser, Subject> {
		constexpr peek_parser(const Subject& subject)
			: unary_parser<cell::peek_parser, Subject>(subject)
		{
		}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			action_state disable_actions{};

			auto copy = first;
			auto result = this->subject.parse(first, last, ctx);
			first = copy;

			return result;
		}
	};

	template <class Subject>
	constexpr inline peek_parser<as_parser_t<Subject>> ahead(Subject const& subject) noexcept {
		return { as_parser(subject) };
	}
}