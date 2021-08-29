#pragma once

#include <climits>
#include <assert.h>

#include "parser.hh"

namespace cell {

	enum : unsigned short {
		BOOST_CC_DIGIT = 0x0001,
		BOOST_CC_XDIGIT = 0x0002,
		BOOST_CC_ALPHA = 0x0004,
		BOOST_CC_CTRL = 0x0008,
		BOOST_CC_LOWER = 0x0010,
		BOOST_CC_UPPER = 0x0020,
		BOOST_CC_SPACE = 0x0040,
		BOOST_CC_PUNCT = 0x0080,
		CC_INLINE_SPACE = 0x0100,
		CC_ODIGIT = 0x0200,
	};

	constexpr const unsigned short ascii_char_types[] =
	{
		/* NUL   0   0 */   BOOST_CC_CTRL,
		/* SOH   1   1 */   BOOST_CC_CTRL,
		/* STX   2   2 */   BOOST_CC_CTRL,
		/* ETX   3   3 */   BOOST_CC_CTRL,
		/* EOT   4   4 */   BOOST_CC_CTRL,
		/* ENQ   5   5 */   BOOST_CC_CTRL,
		/* ACK   6   6 */   BOOST_CC_CTRL,
		/* BEL   7   7 */   BOOST_CC_CTRL,
		/* BS    8   8 */   BOOST_CC_CTRL,
		/* HT    9   9 */   BOOST_CC_CTRL | BOOST_CC_SPACE | CC_INLINE_SPACE,
		/* NL   10   a */   BOOST_CC_CTRL | BOOST_CC_SPACE,
		/* VT   11   b */   BOOST_CC_CTRL | BOOST_CC_SPACE | CC_INLINE_SPACE,
		/* NP   12   c */   BOOST_CC_CTRL | BOOST_CC_SPACE | CC_INLINE_SPACE,
		/* CR   13   d */   BOOST_CC_CTRL | BOOST_CC_SPACE,
		/* SO   14   e */   BOOST_CC_CTRL,
		/* SI   15   f */   BOOST_CC_CTRL,
		/* DLE  16  10 */   BOOST_CC_CTRL,
		/* DC1  17  11 */   BOOST_CC_CTRL,
		/* DC2  18  12 */   BOOST_CC_CTRL,
		/* DC3  19  13 */   BOOST_CC_CTRL,
		/* DC4  20  14 */   BOOST_CC_CTRL,
		/* NAK  21  15 */   BOOST_CC_CTRL,
		/* SYN  22  16 */   BOOST_CC_CTRL,
		/* ETB  23  17 */   BOOST_CC_CTRL,
		/* CAN  24  18 */   BOOST_CC_CTRL,
		/* EM   25  19 */   BOOST_CC_CTRL,
		/* SUB  26  1a */   BOOST_CC_CTRL,
		/* ESC  27  1b */   BOOST_CC_CTRL,
		/* FS   28  1c */   BOOST_CC_CTRL,
		/* GS   29  1d */   BOOST_CC_CTRL,
		/* RS   30  1e */   BOOST_CC_CTRL,
		/* US   31  1f */   BOOST_CC_CTRL,
		/* SP   32  20 */   BOOST_CC_SPACE | CC_INLINE_SPACE,
		/*  !   33  21 */   BOOST_CC_PUNCT,
		/*  "   34  22 */   BOOST_CC_PUNCT,
		/*  #   35  23 */   BOOST_CC_PUNCT,
		/*  $   36  24 */   BOOST_CC_PUNCT,
		/*  %   37  25 */   BOOST_CC_PUNCT,
		/*  &   38  26 */   BOOST_CC_PUNCT,
		/*  '   39  27 */   BOOST_CC_PUNCT,
		/*  (   40  28 */   BOOST_CC_PUNCT,
		/*  )   41  29 */   BOOST_CC_PUNCT,
		/*  *   42  2a */   BOOST_CC_PUNCT,
		/*  +   43  2b */   BOOST_CC_PUNCT,
		/*  ,   44  2c */   BOOST_CC_PUNCT,
		/*  -   45  2d */   BOOST_CC_PUNCT,
		/*  .   46  2e */   BOOST_CC_PUNCT,
		/*  /   47  2f */   BOOST_CC_PUNCT,
		/*  0   48  30 */   BOOST_CC_DIGIT | BOOST_CC_XDIGIT | CC_ODIGIT,
		/*  1   49  31 */   BOOST_CC_DIGIT | BOOST_CC_XDIGIT | CC_ODIGIT,
		/*  2   50  32 */   BOOST_CC_DIGIT | BOOST_CC_XDIGIT | CC_ODIGIT,
		/*  3   51  33 */   BOOST_CC_DIGIT | BOOST_CC_XDIGIT | CC_ODIGIT,
		/*  4   52  34 */   BOOST_CC_DIGIT | BOOST_CC_XDIGIT | CC_ODIGIT,
		/*  5   53  35 */   BOOST_CC_DIGIT | BOOST_CC_XDIGIT | CC_ODIGIT,
		/*  6   54  36 */   BOOST_CC_DIGIT | BOOST_CC_XDIGIT | CC_ODIGIT,
		/*  7   55  37 */   BOOST_CC_DIGIT | BOOST_CC_XDIGIT | CC_ODIGIT,
		/*  8   56  38 */   BOOST_CC_DIGIT | BOOST_CC_XDIGIT,
		/*  9   57  39 */   BOOST_CC_DIGIT | BOOST_CC_XDIGIT,
		/*  :   58  3a */   BOOST_CC_PUNCT,
		/*  ;   59  3b */   BOOST_CC_PUNCT,
		/*  <   60  3c */   BOOST_CC_PUNCT,
		/*  =   61  3d */   BOOST_CC_PUNCT,
		/*  >   62  3e */   BOOST_CC_PUNCT,
		/*  ?   63  3f */   BOOST_CC_PUNCT,
		/*  @   64  40 */   BOOST_CC_PUNCT,
		/*  A   65  41 */   BOOST_CC_ALPHA | BOOST_CC_XDIGIT | BOOST_CC_UPPER,
		/*  B   66  42 */   BOOST_CC_ALPHA | BOOST_CC_XDIGIT | BOOST_CC_UPPER,
		/*  C   67  43 */   BOOST_CC_ALPHA | BOOST_CC_XDIGIT | BOOST_CC_UPPER,
		/*  D   68  44 */   BOOST_CC_ALPHA | BOOST_CC_XDIGIT | BOOST_CC_UPPER,
		/*  E   69  45 */   BOOST_CC_ALPHA | BOOST_CC_XDIGIT | BOOST_CC_UPPER,
		/*  F   70  46 */   BOOST_CC_ALPHA | BOOST_CC_XDIGIT | BOOST_CC_UPPER,
		/*  G   71  47 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  H   72  48 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  I   73  49 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  J   74  4a */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  K   75  4b */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  L   76  4c */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  M   77  4d */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  N   78  4e */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  O   79  4f */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  P   80  50 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  Q   81  51 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  R   82  52 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  S   83  53 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  T   84  54 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  U   85  55 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  V   86  56 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  W   87  57 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  X   88  58 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  Y   89  59 */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  Z   90  5a */   BOOST_CC_ALPHA | BOOST_CC_UPPER,
		/*  [   91  5b */   BOOST_CC_PUNCT,
		/*  \   92  5c */   BOOST_CC_PUNCT,
		/*  ]   93  5d */   BOOST_CC_PUNCT,
		/*  ^   94  5e */   BOOST_CC_PUNCT,
		/*  _   95  5f */   BOOST_CC_PUNCT,
		/*  `   96  60 */   BOOST_CC_PUNCT,
		/*  a   97  61 */   BOOST_CC_ALPHA | BOOST_CC_XDIGIT | BOOST_CC_LOWER,
		/*  b   98  62 */   BOOST_CC_ALPHA | BOOST_CC_XDIGIT | BOOST_CC_LOWER,
		/*  c   99  63 */   BOOST_CC_ALPHA | BOOST_CC_XDIGIT | BOOST_CC_LOWER,
		/*  d  100  64 */   BOOST_CC_ALPHA | BOOST_CC_XDIGIT | BOOST_CC_LOWER,
		/*  e  101  65 */   BOOST_CC_ALPHA | BOOST_CC_XDIGIT | BOOST_CC_LOWER,
		/*  f  102  66 */   BOOST_CC_ALPHA | BOOST_CC_XDIGIT | BOOST_CC_LOWER,
		/*  g  103  67 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  h  104  68 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  i  105  69 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  j  106  6a */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  k  107  6b */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  l  108  6c */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  m  109  6d */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  n  110  6e */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  o  111  6f */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  p  112  70 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  q  113  71 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  r  114  72 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  s  115  73 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  t  116  74 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  u  117  75 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  v  118  76 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  w  119  77 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  x  120  78 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  y  121  79 */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  z  122  7a */   BOOST_CC_ALPHA | BOOST_CC_LOWER,
		/*  {  123  7b */   BOOST_CC_PUNCT,
		/*  |  124  7c */   BOOST_CC_PUNCT,
		/*  }  125  7d */   BOOST_CC_PUNCT,
		/*  ~  126  7e */   BOOST_CC_PUNCT,
		/* DEL 127  7f */   BOOST_CC_CTRL,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	struct ascii
	{
		typedef char char_type;

		static constexpr bool isascii_(int ch)
		{
			return 0 == (ch & ~0x7f);
		}

		static constexpr bool ischar(int ch)
		{
			return isascii_(ch);
		}

		static constexpr bool isalnum(int ch)
		{
			assert(0 == (ch & ~UCHAR_MAX));
			return (ascii_char_types[ch] & BOOST_CC_ALPHA)
				|| (ascii_char_types[ch] & BOOST_CC_DIGIT);
		}

		static constexpr bool isalpha(int ch)
		{
			assert(0 == (ch & ~UCHAR_MAX));
			return (ascii_char_types[ch] & BOOST_CC_ALPHA) ? true : false;
		}

		static constexpr bool isdigit(int ch)
		{
			assert(0 == (ch & ~UCHAR_MAX));
			return (ascii_char_types[ch] & BOOST_CC_DIGIT) ? true : false;
		}

		static constexpr bool isxdigit(int ch)
		{
			assert(0 == (ch & ~UCHAR_MAX));
			return (ascii_char_types[ch] & BOOST_CC_XDIGIT) ? true : false;
		}

		static constexpr bool isodigit(int ch)
		{
			assert(0 == (ch & ~UCHAR_MAX));
			return (ascii_char_types[ch] & CC_ODIGIT) ? true : false;
		}

		static constexpr bool iscntrl(int ch)
		{
			assert(0 == (ch & ~UCHAR_MAX));
			return (ascii_char_types[ch] & BOOST_CC_CTRL) ? true : false;
		}

		static constexpr bool isgraph(int ch)
		{
			return ('\x21' <= ch && ch <= '\x7e');
		}

		static constexpr bool islower(int ch)
		{
			assert(0 == (ch & ~UCHAR_MAX));
			return (ascii_char_types[ch] & BOOST_CC_LOWER) ? true : false;
		}

		static constexpr bool isprint(int ch)
		{
			return ('\x20' <= ch && ch <= '\x7e');
		}

		static constexpr bool ispunct(int ch)
		{
			assert(0 == (ch & ~UCHAR_MAX));
			return (ascii_char_types[ch] & BOOST_CC_PUNCT) ? true : false;
		}

		static constexpr bool isspace(int ch)
		{
			assert(0 == (ch & ~UCHAR_MAX));
			return (ascii_char_types[ch] & BOOST_CC_SPACE) ? true : false;
		}

		static constexpr bool isinlspace(int ch)
		{
			assert(0 == (ch & ~UCHAR_MAX));
			return (ascii_char_types[ch] & CC_INLINE_SPACE) ? true : false;
		}

#define PREVENT_MACRO_SUBSTITUTION
		static constexpr bool isblank PREVENT_MACRO_SUBSTITUTION(int ch)
		{
			return ('\x09' == ch || '\x20' == ch);
		}

		static constexpr bool isupper(int ch)
		{
			assert(0 == (ch & ~UCHAR_MAX));
			return (ascii_char_types[ch] & BOOST_CC_UPPER) ? true : false;
		}
	};

	template <typename Derived>
	struct basic_is_a : character_parser<Derived> {
		constexpr basic_is_a() = default;
		/*overridable*/ constexpr bool is_a(int) const noexcept { return false; }
		constexpr bool is(int c) const noexcept {
			return this->derived().is_a(c);
		}

		template <typename Iterator, typename Context>
		bool parse(Iterator& first, const Iterator& last, Context& ctx) const {
			this->filter(first, last, ctx);
			if (first != last && is(*first)) {
				++first;
				return true;
			}
			return false;
		}
	};

#define IS_AN(klass) \
	struct is_an_ ## klass : basic_is_a<is_an_ ## klass> { \
		constexpr is_an_ ## klass() = default; \
		constexpr bool is_a(int c) const noexcept { return ascii::is##klass(c); } \
	}; \
	constexpr auto klass = is_an_ ## klass {}

#define IS_A(klass) \
	struct is_a_ ## klass : basic_is_a<is_a_ ## klass> { \
		constexpr is_a_ ## klass() = default; \
		constexpr bool is_a(int c) const noexcept { return ascii::is##klass(c); } \
	}; \
	constexpr auto klass = is_a_ ## klass {}

	IS_AN(inlspace);
	IS_AN(alpha);
	IS_AN(alnum);
	IS_A(digit);
	IS_AN(xdigit);
	IS_AN(odigit);
}