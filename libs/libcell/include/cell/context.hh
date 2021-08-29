#pragma once
#include <string_view>

namespace cell {
	template <typename Iterator, typename Filter, typename Destination>
	struct context {
		using iterator = Iterator;

		Filter filter;
		Destination dest;
		std::pair<iterator, iterator> range;
		explicit context(const Filter& filter, const Destination& dest)
			: filter{ filter }
			, dest{ dest }
		{}
	};

	template <typename Iterator, typename Filter, typename Destination>
	inline void _setrange(const Iterator& first, const Iterator& last, context<Iterator, Filter, Destination>& ctx) {
		ctx.range = std::make_pair(first, last);
	}

	template <typename Iterator, typename Filter, typename Destination>
	inline auto const& _getrange(context<Iterator, Filter, Destination>& ctx) {
		return ctx.range;
	}

	template <typename Iterator, typename Filter, typename Destination>
	inline Destination& _val(context<Iterator, Filter, Destination>& ctx) { return ctx.dest; }

	template <typename Iterator, typename Filter, typename Destination>
	inline Filter const& _filter(context<Iterator, Filter, Destination>& ctx) { return ctx.filter; }

	template <typename Iterator, typename Filter, typename Destination>
	inline std::string_view _view(context<Iterator, Filter, Destination>& ctx) {
		auto const length = static_cast<size_t>(std::distance(ctx.range.first, ctx.range.second));
		return { &*ctx.range.first, length };
	}
}