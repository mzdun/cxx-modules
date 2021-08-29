#pragma once

#include "cell/context.hh"
#include "cell/parser.hh"
#include "cell/special.hh"

#define RULE_MAP(name) \
	struct on_ ## name ## _handler { \
		constexpr on_ ## name ## _handler() = default; \
		template <class Context> void operator()(Context& context) const; \
	}; \
	constexpr auto on_ ## name = on_ ## name ## _handler{};\
	template <class Context>\
	void on_ ## name ## _handler::operator()([[maybe_unused]] Context& context) const

#define RULE_EMIT(name, tok) RULE_MAP(name) { _emit(context, tok); }

namespace hl {
	struct endline_t {
		size_t offset;
		size_t size;
		bool operator < (const endline_t& rhs) const {
			return offset < rhs.offset;
		}
		bool operator == (const endline_t& rhs) const {
			return offset == rhs.offset;
		}
	};
	using endlines = std::vector<endline_t>;

	class grammar_result {
		endlines endlines_{ { 0 , 0 } };
		tokens tokens_;

	public:
		void emit(std::size_t start, std::size_t end, token kind) {
			switch (kind) {
			/*case hl::cxx::token::deleted_newline:
				eol(start + 1, end);
				return;*/
			case token::newline:
				endlines_.push_back({ end, end - start });
				return;
			default:
				tokens_.push_back({ start, end, kind });
				break;
			};
		}

		void produce_lines(callback& cb, size_t contents_length);
	};

	template <typename Iterator>
	class grammar_value {
		grammar_result* ref_{ nullptr };
		Iterator begin_{};
	public:
		grammar_value() = default;
		grammar_value(grammar_result* ref, Iterator begin) : ref_{ ref }, begin_{ begin } {}

		template <typename Context>
		void emit(Context& context, token kind)
		{
			auto const start = static_cast<size_t>(std::distance(begin_, context.range.first));
			auto const end = static_cast<size_t>(std::distance(begin_, context.range.second));
			ref_->emit(start, end, kind);
		}
	};

	template <typename Context, typename Token>
	void _emit(Context& context, Token kind) {
		_val(context).emit(context, static_cast<token>(kind));
	}

	template <template <class> class Value, typename Iterator, typename Parser, typename Filter>
	void parse_with_restart(Iterator begin, const Iterator& end, const Parser& code_parser, const Filter& filter_parser, grammar_result& result) {
		using value_t = Value<Iterator>;
		using filter_t = cell::as_parser_t<Filter>;
		using context_t = cell::context<Iterator, filter_t, value_t>;

		value_t val{ &result, begin };
		auto ctx = context_t{ as_parser(filter_parser), val };

		while (begin != end) {
			(void)as_parser(code_parser).parse(begin, end, ctx);
			while (begin != end && !cell::ahead(cell::eol).parse(begin, end, ctx))
				++begin;
		}
	}
}
