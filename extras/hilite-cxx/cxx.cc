#include "hilite/cxx.hh"
#include "hilite/hilite_impl.hh"

#include "cell/ascii.hh"
#include "cell/character.hh"
#include "cell/context.hh"
#include "cell/operators.hh"
#include "cell/parser.hh"
#include "cell/repeat_operators.hh"
#include "cell/special.hh"
#include "cell/string.hh"
#include "cell/tokens.hh"

#include <limits.h>
#include <functional>
#include <string>
#include <string_view>

namespace hl::cxx::parser::callbacks {
	template <typename Iterator>
	struct cxx_grammar_value : grammar_value<Iterator> {
		using grammar_value<Iterator>::grammar_value;
		bool is_raw{false};
	};

	template <typename Context>
	auto& _is_raw_string(Context& context) {
		return _val(context).is_raw;
	}

	RULE_EMIT(newline, token::newline)
	RULE_EMIT(deleted_eol, token::deleted_newline)
	RULE_EMIT(block_comment, token::block_comment)
	RULE_EMIT(line_comment, token::line_comment)
	RULE_EMIT(ws, token::whitespace)
	RULE_EMIT(character_literal, token::character)
	RULE_EMIT(string_literal,
	          _is_raw_string(context) ? token::raw_string : token::string)
	RULE_EMIT(punctuator, token::punctuator)
	RULE_EMIT(pp_identifier, token::preproc_identifier)
	RULE_EMIT(macro_name, token::macro_name)
	RULE_EMIT(macro_arg, token::macro_arg)
	RULE_EMIT(macro_va_args, token::macro_va_args)
	RULE_EMIT(replacement, token::macro_replacement)
	RULE_EMIT(control_line, token::preproc)
	RULE_EMIT(system_header, token::system_header_name)
	RULE_EMIT(local_header, token::local_header_name)
	RULE_EMIT(pp_number, token::number)
	RULE_EMIT(escaped, token::escape_sequence)
	RULE_EMIT(ucn, token::universal_character_name)
	RULE_EMIT(pp_define_arg_list, token::macro_arg_list)

	RULE_EMIT(char_encoding, token::char_encoding)
	RULE_EMIT(char_delim, token::char_delim)
	RULE_EMIT(char_udl, token::char_udl)

	RULE_EMIT(string_encoding, token::string_encoding)
	RULE_EMIT(string_delim, token::string_delim)
	RULE_EMIT(string_udl, token::string_udl)

	RULE_EMIT(module_name, token::module_name)
	RULE_EMIT(module_export, token::module_export)
	RULE_EMIT(import, token::module_import)
	RULE_EMIT(module_decl, token::module_decl)
	RULE_EMIT(module_part_marker, token::punctuator)

	using namespace std::literals;
	auto find_eol_end(const std::string_view& sv, size_t pos) {
		auto len = sv.size();
		bool matched = false;
		if (pos < len && sv[pos] == '\r') {
			++pos;
			matched = true;
		}
		if (pos < len && sv[pos] == '\n') {
			++pos;
			matched = true;
		}
		return matched ? pos : std::string_view::npos;
	}

	size_t find_del_eol(const std::string_view& sv, size_t pos = 0) {
		while (true) {
			auto slash = sv.find('\\', pos);
			if (slash == std::string_view::npos) return std::string_view::npos;
			auto end = find_eol_end(sv, slash + 1);
			if (end != std::string_view::npos) return end;
			++slash;
		}
	}

	std::string remove_deleted_eols(const std::string_view& sv) {
		auto pos = find_del_eol(sv);
		auto prev = decltype(pos){};
		if (pos == std::string_view::npos) return {};

		std::string out;
		out.reserve(sv.length());

		while (pos != std::string_view::npos) {
			auto until = pos;
			while (sv[until] != '\\')
				--until;

			out.append(sv.data() + prev, sv.data() + until);

			prev = pos;
			pos = find_del_eol(sv, pos);
		}

		out.append(sv.data() + prev, sv.data() + sv.length());
		return out;
	}

	RULE_MAP(identifier) { _emit(context, token::identifier); }
}  // namespace hl::cxx::parser::callbacks

namespace hl::cxx::parser {
	using namespace hl::cxx::parser::callbacks;
	using namespace cell;

	static constexpr auto operator""_ident(const char* str, size_t len) {
		return string_token{std::string_view(str, len)}[on_identifier];
	}

	static constexpr auto operator""_pp_ident(const char* str, size_t len) {
		return string_token{std::string_view(str, len)}[on_pp_identifier];
	}

	// clang-format off
	constexpr auto line_comment =
		("//" >> *(ch - eol))
		;

	constexpr auto block_comment =
		(
		"/*" >> *(
			(ch - '*' - eol)
			| ('*' >> !ch('/'))
			| eol									[on_newline]
			) >> "*/"
		)
		;

	constexpr auto SP_char =
		inlspace
		| line_comment
		| block_comment
		;

	constexpr auto SP =
		*SP_char
		;

	constexpr auto mandatory_SP =
		+SP_char
		;

	constexpr auto UCN_value =
		('u' >> repeat(4)(xdigit))
		| ('U' >> repeat(8)(xdigit))
		;

	constexpr auto nondigit =
		'_'
		| alpha
		;

	constexpr auto ident_nondigit =
		'_'
		| alpha
		| ('\\' >> UCN_value)
		;

	constexpr auto sign = ch("+-");

	constexpr auto header_name =
		('<' >> +(ch - eol - '>') >> '>')			[on_system_header]
		| ('"' >> +(ch - eol - '"') >> '"')			[on_local_header]
		;

	constexpr auto identifier =
		ident_nondigit
		>> *(ident_nondigit | digit)
		;

	constexpr auto pp_number =
		-ch('.') >> digit >> *(
			digit
			| ident_nondigit
			| ('\'' >> (digit | nondigit))
			| (ch("eEpP") >> sign)
			)
		;
	// clang-format on

	template <char C>
	struct cxx_char_parser : cell::character_parser<cxx_char_parser<C>> {
		constexpr cxx_char_parser() = default;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			this->filter(first, last, ctx);

			if (first == last) return false;

			auto save = first;

			{
				cell::action_state disable_actions{};
				if (eol.parse(first, last, ctx)) {
					first = save;
					return false;
				}
			}

			if (*first == C) {
				first = save;
				return false;
			}

			bool escaped = *first == '\\';
			++first;

			if (!escaped) return true;

			this->filter(first, last, ctx);

			if (first == last) {
				first = save;
				return false;
			}

			auto const escaped_ch = *first;
			++first;

			switch (escaped_ch) {
				case 'u':
					if (repeat(4)(xdigit).parse(first, last, ctx)) {
						if (cell::action_state::enabled()) {
							_setrange(save, first, ctx);
							on_ucn(ctx);
						}
						return true;
					}
					break;
				case 'U':
					if (repeat(8)(xdigit).parse(first, last, ctx)) {
						if (cell::action_state::enabled()) {
							_setrange(save, first, ctx);
							on_ucn(ctx);
						}
						return true;
					}
					break;
				case 'x':
					if ((+xdigit).parse(first, last, ctx)) {
						if (cell::action_state::enabled()) {
							_setrange(save, first, ctx);
							on_escaped(ctx);
						}
						return true;
					}
					break;
				case '0':
					if ((*odigit).parse(first, last, ctx)) {
						if (cell::action_state::enabled()) {
							_setrange(save, first, ctx);
							on_escaped(ctx);
						}
						return true;
					}
					break;
				default:
					if (cell::action_state::enabled()) {
						_setrange(save, first, ctx);
						on_escaped(ctx);
					}
					return true;
			}

			first = save;
			return false;
		}
	};

	// clang-format off
	constexpr auto c_char = cxx_char_parser<'\''>{};
	constexpr auto s_char = cxx_char_parser<'"'>{};

	constexpr auto d_char =
		ch - ch(" ()\\\t\v\f\r\n\"")
		;

	constexpr auto encoding_prefix =
		lit("u8")
		| "u"
		| "U"
		| "L"
		;

	constexpr auto character_literal =
		-(encoding_prefix)
		>> ch('\'') >> +c_char >> ch('\'')
		>> -(identifier)
		;
	// clang-format on

	struct cxx_string_parser : cell::parser<cxx_string_parser> {
		constexpr cxx_string_parser() = default;
		template <typename Iterator>
		using range_t = std::pair<Iterator, Iterator>;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			Iterator start_delim_end, finish_delim_begin;

			_is_raw_string(ctx) = false;

			auto get_quot_end = [&start_delim_end](auto& ctx) {
				start_delim_end = _getrange(ctx).second;
			};
			auto get_quot_start = [&finish_delim_begin](auto& ctx) {
				finish_delim_begin = _getrange(ctx).first;
			};
			auto c_string =
			    (ch('"')[get_quot_end]) >> *s_char >> (ch('"')[get_quot_start]);
			auto copy = first;
			(void)encoding_prefix.parse(first, last, ctx);
			auto result = false;
			auto is_raw = ch('R').parse(first, last, ctx);
			if (is_raw)
				result = parse_raw(first, last, ctx, start_delim_end,
				                   finish_delim_begin);
			else
				result = c_string.parse(first, last, ctx);

			if (!result) {
				first = copy;
				return false;
			}

			(void)identifier.parse(first, last, ctx);
			return true;
		}

		template <typename Iterator, typename Context>
		bool parse_raw(Iterator& first,
		               const Iterator& last,
		               Context& ctx,
		               Iterator& start_delim_end,
		               Iterator& finish_delim_begin) const {
			if (!ch('"').parse(first, last, ctx)) return false;

			_is_raw_string(ctx) = true;
			auto const front_delim_start = first;
			while (d_char.parse(first, last, ctx))
				;
			auto const front_delim_end = first;

			if (!ch('(').parse(first, last, ctx)) return false;

			start_delim_end = first;

			while (first != last) {
				while (first != last) {
					static constexpr auto signaling_eol = eol[on_newline];
					if (signaling_eol.parse(first, last, ctx)) continue;
					auto copy = first;
					if (ch(')').parse(first, last, ctx)) {
						finish_delim_begin = copy;
						break;
					}
					++first;
				}
				// while (first != last && !ch(')').parse(first, last, ctx))
				// ++first;
				if (!parse_end_delim(front_delim_start, front_delim_end, first,
				                     last))
					continue;

				auto copy = first;
				if (ch('"').parse(copy, last, ctx)) {
					first = copy;
					return true;
				}
			}
			return false;
		}

		template <typename Iterator>
		bool parse_end_delim(Iterator delim_first,
		                     const Iterator& delim_last,
		                     Iterator& first,
		                     const Iterator& last) const {
			if (delim_first == delim_last) return true;

			while ((delim_first != delim_last) && (first != last)) {
				if (*delim_first != *first) return false;
				++delim_first;
				++first;
			}
			return true;
		}
	};

	// clang-format off
	constexpr auto string_literal = cxx_string_parser{};

	constexpr auto operators =
		lit("...") | "<=>" | "<<=" | ">>=" | "<<" | ">>" |
		"<:" | ":>" | "<%" | "%>" | "%:%:" | "%:" |
		"::" | "->*" | "->" | ".*" |
		"+=" | "-=" | "*=" | "/=" | "%=" | "^=" | "&=" | "|=" |
		"++" | "--" | "==" | "!=" | "<=" | ">=" | "&&" | "||" | "##" |
		ch("<>{}[]#()=;:?.~!+-*/%^&|,");

	constexpr auto preprocessing_token =
		character_literal							[on_character_literal]
		| string_literal							[on_string_literal]
		| identifier								[on_identifier]
		| pp_number									[on_pp_number]
		| operators									[on_punctuator]
		;

	constexpr auto mSP = mandatory_SP;
	constexpr auto __has_include_token =
		"__has_include"_ident
		>> SP
		>> '('
		>> SP >> (
			header_name
			| string_literal
			| ('<'
				>> SP
				>> +((preprocessing_token - '>') >> SP)
				>> SP
				>> '>'
				)
			)
		>> SP
		>> ')'
		;

	constexpr auto constant_expression =
		+(
			(
			__has_include_token
			| preprocessing_token
			)
			>> SP
		)
		;

	constexpr auto opt_pp_tokens =
		-(mSP
			>> preprocessing_token
			>> *(SP >> preprocessing_token)
			)
		>> SP
		;
	constexpr auto pp_define_arg_list =
		( '('
			>> SP
			>> (
				lit("...")							[on_macro_va_args]
				| identifier						[on_macro_arg]
					>> *(
						SP
						>> ','
						>> SP
						>> identifier				[on_macro_arg]
						)
					>> -(
						SP
						>> ','
						>> SP
						>> lit("...")				[on_macro_va_args]
						)
				| eps
				)
			>> SP
			>> ')'
			)										[on_pp_define_arg_list]
		;
	// clang-format on

	template <typename Wrapped>
	struct wrapped : cell::parser<wrapped<Wrapped>>, Wrapped {
		constexpr wrapped(const Wrapped& inner) : Wrapped(inner) {}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			return Wrapped::operator()().parse(first, last, ctx);
		}
	};

	template <typename Wrapped>
	constexpr wrapped<Wrapped> wrap(const Wrapped& inner) {
		return inner;
	}

	// clang-format off
	constexpr auto pp_include = wrap([] {
		static constexpr auto grammar =
			"include"_pp_ident
			>> SP
			>> +((header_name | preprocessing_token) >> SP);
		return (grammar);
	});

	constexpr auto pp_define = wrap([] {
		static constexpr auto grammar =
			"define"_pp_ident
			>> mSP
			>> identifier							[on_macro_name]
			>> -pp_define_arg_list
			>> SP
			>> (*(preprocessing_token >> SP))		[on_replacement]
			;
		return (grammar);
	});
	constexpr auto pp_undef = wrap([] {
		static constexpr auto grammar =
			"undef"_pp_ident
			>> mSP
			>> identifier							[on_macro_name]
			>> SP
			;
		return (grammar);
	});
	constexpr auto pp_if = wrap([] {
		static constexpr auto grammar =
			"if"_pp_ident
			>> mSP
			>> constant_expression
			;
		return (grammar);
	});
	constexpr auto pp_ifdef = wrap([] {
		static constexpr auto grammar =
			"ifdef"_pp_ident
			>> mSP
			>> identifier							[on_macro_name]
			>> SP
			;
		return (grammar);
	});
	constexpr auto pp_ifndef = wrap([] {
		static constexpr auto grammar =
			"ifndef"_pp_ident
			>> mSP
			>> identifier							[on_macro_name]
			>> SP
			;
		return (grammar);
	});

	// at this point, this is too much for MSVC, it will not treat whatever it sees as
	// constexpression anymore.

	struct shorten_typename : cell::parser<shorten_typename> {
		static constexpr auto parser =
			pp_include
			| pp_define
			| pp_undef
			| pp_ifdef
			| pp_ifndef
			| pp_if
			| ("elif"_pp_ident >> mSP >> constant_expression)
			| "else"_pp_ident >> SP
			| "endif"_pp_ident >> SP
			| ("line"_pp_ident >> mSP >> +(preprocessing_token >> SP))
			| ("error"_pp_ident >> opt_pp_tokens)
			| ("pragma"_pp_ident >> opt_pp_tokens)
			| opt_pp_tokens
			;

		constexpr shorten_typename() = default;
		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			return parser.parse(first, last, ctx);
		}
	};

	constexpr auto pp_control = shorten_typename{};

	constexpr auto pp_module_name_ident =
		identifier                                                          [on_identifier]
		;

	constexpr auto pp_qualified_name = 
		pp_module_name_ident >>
			*(SP
				>> ch('.')                                                  [on_punctuator]
				>> SP
				>> pp_module_name_ident)
		;

	constexpr auto pp_module_part_marker =
		ch(':')                                                             [on_punctuator]
		;

	constexpr auto pp_module_ref =
		(pp_qualified_name >> SP >>
			pp_module_part_marker >> SP >> pp_qualified_name)               [on_module_name]
		| pp_qualified_name                                                 [on_module_name]
		| (pp_module_part_marker >> SP >> pp_qualified_name)                [on_module_name]
		;

	constexpr auto pp_import =
		(
			lit("import")
			>> SP
			>> (header_name
				| (pp_module_ref >> SP >> *((preprocessing_token - ch(';')) >> SP) >> ch(';'))
				)
		)                                                                   [on_import]
		;

	constexpr auto pp_module =
		(
			lit("module")
			>> SP
			>> -pp_module_ref
			>> SP
			>> *((preprocessing_token - ch(';')) >> SP)
			>> ch(';')
		)                                                                   [on_module_decl]
		;

	constexpr auto control_line =
		('#' >> SP >> pp_control)											[on_control_line]
		| (lit("export") >> SP >> (pp_module | pp_import))                  [on_module_export]
		| pp_module
		| pp_import
		;

	constexpr auto text_line = *(preprocessing_token >> SP);
	constexpr auto line = SP >> (control_line | text_line) >> SP;
	constexpr auto preprocessing_file =
		*(
			ahead(line >> eol)
			>> line
			>> eol		                                                    [on_newline]
			)
		>> -line
		;

	struct deleted_eol_parser : cell::parser<deleted_eol_parser> {
		constexpr deleted_eol_parser() = default;

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			auto const emit0 = first;
			if (first != last && *first == '\\') {
				++first;
				auto const emit1 = first;
				if (eol.parse(first, last, ctx)) {
					if (cell::action_state::enabled()) {
						_setrange(emit0, emit1, ctx);
						on_deleted_eol(ctx);

						_setrange(emit1, first, ctx);
						on_newline(ctx);
					}
					return true;
				};
			}
			return false;
		}
	};

	constexpr auto deleted_eol = deleted_eol_parser{};
	// clang-format on
}  // namespace hl::cxx::parser

namespace hl::cxx {
	using namespace hl::cxx::parser;

	std::string_view token_to_string(unsigned tok) noexcept {
		using namespace std::literals;

#define STRINGIFY(x)        \
	case hl::cxx::token::x: \
		return #x##sv;
		switch (tok) {
			CXX_HILITE_TOKENS(STRINGIFY)
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

		parse_with_restart<cxx_grammar_value>(begin, end, preprocessing_file,
		                                      deleted_eol, value);
		value.produce_lines(result, contents.size());
	}
}  // namespace hl::cxx
