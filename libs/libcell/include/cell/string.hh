#pragma once
#include <string_view>

#include "parser.hh"

namespace cell {
	class string_token : public character_parser<string_token> {
		std::string_view value_{};
	public:
		constexpr string_token() = default;
		constexpr string_token(std::string_view sv) noexcept : value_{ std::move(sv) } {}
		template <size_t Length>
		constexpr string_token(const char(&view)[Length]) noexcept : value_{ view, Length - 1 } {}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			auto copy = first;
			for (const auto rhs : value_) {
				this->filter(copy, last, ctx);
				if (copy == last || *copy != rhs)
					return false;
				++copy;
			}
			first = copy;
			return true;
		}
	};

	template <size_t Length>
	constexpr string_token lit(const char(&view)[Length]) {
		return std::string_view(view, Length - 1);
	}

	namespace ext {
		template <size_t Length>
		struct as_parser<char[Length]> {
			using type = string_token;
		};
		template <>
		struct as_parser<std::string_view> {
			using type = string_token;
		};
	}

}
