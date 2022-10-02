#include "cxx/scanner.hh"
#include <base/utils.hh>
#include <hilite/cxx.hh>
#include <algorithm>

using namespace std::literals;

namespace {
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
		if (pos == std::string_view::npos) return as_str(sv);

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

	struct decl_info {
		bool module_export{false};
		bool module_decl{false};
		bool module_import{false};
		bool legacy_header{false};
		size_t name_start{}, name_end{};

		bool is_decl(hl::token_t const& tok) {
			switch (static_cast<hl::cxx::token>(tok.kind)) {
				case hl::cxx::module_export:
					module_export = true;
					return true;
				case hl::cxx::module_decl:
					module_decl = true;
					return true;
				case hl::cxx::module_import:
					module_import = true;
					return true;
				case hl::cxx::module_name:
					name_start = tok.start;
					name_end = tok.end;
					return true;
				case hl::cxx::system_header_name:
				case hl::cxx::local_header_name:
					legacy_header = true;
					name_start = tok.start;
					name_end = tok.end;
					return false;
				default:
					break;
			}
			return false;
		}

		bool within(hl::token_t const& tok) const {
			auto const result = tok.start >= name_start && tok.start < name_end;
			return result;
		}

		void filter(hl::tokens& tokens) {
			auto it = std::remove_if(tokens.begin(), tokens.end(),
			                         [self = this](hl::token_t const& tok) {
				                         return self->is_decl(tok);
			                         });
			it = std::remove_if(tokens.begin(), it,
			                    [self = this](hl::token_t const& tok) {
				                    return !self->within(tok);
			                    });
			tokens.erase(it, tokens.end());
			std::stable_sort(
			    tokens.begin(), tokens.end(),
			    [](hl::token_t const& lhs, hl::token_t const& rhs) {
				    return lhs.start < rhs.start;
			    });
		}
	};

	static constexpr struct {
		std::string_view open;
		std::string_view close;
	} brackets[] = {
	    {"{"sv, "}"sv},
	    {"("sv, ")"sv},
	};

	struct callback : hl::callback {
		std::string_view text{};
		std::vector<std::string_view> close_parens;
		module_unit& result;

		explicit callback(std::string_view text, module_unit& result)
		    : text{text}, result{result} {}

		static bool is_module_decl(hl::tokens const& highlights) noexcept {
			if (highlights.empty()) return false;
			switch (static_cast<hl::cxx::token>(highlights.front().kind)) {
				case hl::cxx::module_export:
				case hl::cxx::module_import:
				case hl::cxx::module_decl:
					return true;
				default:
					break;
			}
			return false;
		}

		void on_line(std::size_t start,
		             std::size_t length,
		             hl::tokens const& highlights) {
			if (close_parens.empty() && is_module_decl(highlights)) {
				on_module(start, length, highlights);
				return;
			}

			auto line = text.substr(start, length);
			for (auto const& tok : highlights) {
				switch (tok.kind) {
					case hl::punctuator: {
						auto punc = remove_deleted_eols(
						    line.substr(tok.start, tok.end - tok.start));

						for (auto const& [open, close] : brackets) {
							if (punc == close) {
								pop(close);
								break;
							}
							if (punc == open) {
								push(close);
								break;
							}
						}

						break;
					}
					default:
						break;
				}
			}
		}

		void on_module(std::size_t start,
		               std::size_t length,
		               hl::tokens tokens) {
			decl_info info{};
			info.filter(tokens);
			if (tokens.empty()) return;

			std::u8string module_name, part_name;
			auto dest = &module_name;

			auto line = text.substr(start, length);
			for (auto const& tok : tokens) {
				switch (static_cast<hl::cxx::token>(tok.kind)) {
					case hl::cxx::identifier:
					case hl::cxx::system_header_name:
					case hl::cxx::local_header_name:
						dest->append(as_u8sv(remove_deleted_eols(
						    line.substr(tok.start, tok.end - tok.start))));
						break;

					case hl::cxx::punctuator: {
						if (tok.end - tok.start > 1) return;
						auto punc = line[tok.start];
						if (punc == ':') {
							if (dest == &part_name) return;
							dest = &part_name;
							break;
						}
						if (punc == '.') dest->push_back('.');
						break;
					}

					default:
						return;
				}
			}

			if (info.module_decl) {
				result.is_interface = info.module_export;
				if (!info.module_export) {
					result.imports.push_back({module_name, part_name});
				}
				result.name.module = std::move(module_name);
				result.name.part = std::move(part_name);
				return;
			}

			if (info.module_import) {
				if (info.legacy_header) {
					if (!module_name.empty() && part_name.empty()) {
						result.imports.push_back({std::move(module_name), {}});
					}
					return;
				}
				result.imports.push_back(
				    {std::move(module_name), std::move(part_name)});
				return;
			}
		}

		void push(std::string_view close) { close_parens.push_back(close); }
		void pop(std::string_view close) {
			if (!close_parens.empty()) {
				if (close_parens.back() == close) {
					close_parens.pop_back();
				} else {
					auto index = close_parens.size();
					for (; index > 0; --index) {
						if (close != close_parens[index - 1]) continue;
						close_parens.resize(index - 1);
					}
				}
			}
		}
	};

}  // namespace

module_unit cxx::scan(std::string_view text) {
	module_unit unit{};
	callback cb{text, unit};
	hl::cxx::tokenize(cb.text, cb);

	for (auto& import : unit.imports) {
		if (import.part.empty()) continue;
		if (!import.module.empty() || unit.name.module.empty()) {
			import.module.clear();
			import.part.clear();
			continue;
		}
		import.module = unit.name.module;
	}

	auto it =
	    std::remove_if(unit.imports.begin(), unit.imports.end(),
	                   [](auto& import) { return import.module.empty(); });
	unit.imports.erase(it, unit.imports.end());

	return unit;
}
