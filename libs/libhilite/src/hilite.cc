#include "hilite/hilite.hh"
#include "hilite/hilite_impl.hh"

#include <algorithm>

namespace hl {
	namespace {
		template <typename Vector>
		void sort_uniq(Vector& v) {
			std::sort(std::begin(v), std::end(v));
			auto last = std::unique(std::begin(v), std::end(v));
			v.erase(last, std::end(v));
		}

		void break_token(tokens& out,
			token_t tok,
			endlines::const_iterator eol,
			endlines::const_iterator const& eol_to)
		{
			while (eol != eol_to) {
				auto const end = eol->offset - eol->size;
				if (tok.end <= end) {
					out.push_back({ tok.start, tok.end, tok.kind });
					return;
				}
				out.push_back({ tok.start, end, tok.kind });
				tok.start = eol->offset;
				++eol;
			}
			out.push_back({ tok.start, tok.end, tok.kind });
		}

		tokens break_lines(
			endlines::const_iterator const& eol_from,
			endlines::const_iterator const& eol_to,
			tokens::const_iterator from,
			tokens::const_iterator const& to)
		{
			tokens out;
			out.reserve(static_cast<size_t>(std::distance(from, to)));

			for (auto eol = eol_from; eol != eol_to; ++eol) {
				while (from != to && from->start < eol->offset) {
					break_token(out, *from, eol, eol_to);
					++from;
				}
			}

			out.insert(out.end(), from, to);

			return out;
		}
	}

	callback::~callback() = default;
	callback::callback() = default;
	callback::callback(callback&&) = default;
	callback& callback::operator=(callback&&) = default;

	void grammar_result::produce_lines(callback& cb, size_t contents_length) {
		sort_uniq(endlines_);
		sort_uniq(tokens_);

		auto broken = break_lines(
			std::begin(endlines_), std::end(endlines_),
			std::begin(tokens_), std::end(tokens_));
		std::sort(std::begin(broken), std::end(broken));
		tokens_.clear();
		for (size_t i = 1; i < endlines_.size(); ++i) {
			auto& prev = endlines_[i - 1];
			auto const& curr = endlines_[i];
			prev.size = curr.offset - prev.offset - curr.size;
		}

		{
			auto & back = endlines_.back();
			back.size = contents_length < back.offset ? 0 : contents_length - back.offset;
		}

		auto it = std::begin(broken);
		auto const end = std::end(broken);
		for (auto const& endline : endlines_) {
			auto const eol = endline.offset + endline.size;

			auto saved_pos = it;
			size_t token_count = 0;
			size_t prev_end = 0;
			bool prev_ws = false;

			while (it != end && it->end <= eol) {
				++token_count;
				const bool is_ws = it->kind == hl::token::whitespace;
				if (prev_ws && is_ws && it->start == prev_end)
					--token_count;
				prev_ws = is_ws;
				prev_end = it->end;
				++it;
			}

			tokens line_tokens;
			line_tokens.reserve(token_count);

			prev_end = 0;
			prev_ws = false;
			it = saved_pos;

			while (it != end && it->end <= eol) {
				const bool is_ws = it->kind == hl::token::whitespace;
				if (prev_ws && is_ws && it->start == prev_end) {
					line_tokens.back().end = it->end;
				}
				else {
					line_tokens.push_back({ it->start, it->end, it->kind });
				}

				prev_ws = is_ws;
				prev_end = it->end;
				++it;
			}

			for (auto& tok : line_tokens) {
				tok.start -= endline.offset;
				tok.end -= endline.offset;
			}

			cb.on_line(endline.offset, endline.size, line_tokens);
		}
	}
}
