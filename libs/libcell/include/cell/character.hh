#pragma once
#include <string_view>

#include "parser.hh"

namespace cell {
	struct literal_character : character_parser<literal_character> {
		char value{};

		constexpr literal_character() = default;
		constexpr literal_character(char c) noexcept : value{ c } {}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			this->filter(first, last, ctx);
			if (first != last && *first == value) {
				++first;
				return true;
			}
			return false;
		}
	};

	namespace ext {
		template <>
		struct as_parser<char> {
			using type = literal_character;
		};
	}

	class one_of_characters : public character_parser<one_of_characters> {
		std::string_view value_{};
	public:
		constexpr one_of_characters() = default;
		constexpr one_of_characters(std::string_view sv) noexcept : value_{ std::move(sv) } {}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			this->filter(first, last, ctx);
			if (first != last) {
				const auto lhs = *first;
				for (const auto rhs : value_) {
					if (lhs == rhs) {
						++first;
						return true;
					}
				}
			}
			return false;
		}
	};

	class any_character : public character_parser<any_character> {
	public:
		constexpr any_character() = default;

		constexpr literal_character operator()(char c) const noexcept {
			return c;
		}

		constexpr literal_character operator()(const char(&c)[2]) const noexcept {
			return c[0];
		}

		template <size_t Length>
		constexpr one_of_characters operator()(const char(&view)[Length]) const noexcept {
			return std::string_view(view, Length - 1);
		}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			this->filter(first, last, ctx);
			if (first != last) {
				++first;
				return true;
			}
			return false;
		}
	};

	constexpr auto ch = any_character{};
}
