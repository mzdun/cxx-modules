#include "xml/handlers-internal.hh"

namespace xml {
	void var_handler::onElement(xml_config&, char const** attrs) {
		for (auto attr = attrs; *attr; attr += 2) {
			auto const name = std::string_view{attr[0]};
			if (name == "name"sv)
				parent->current.args.push_back(named_var{attr[1]});
		}
	}

	void cxx_handler::onElement(xml_config&, char const**) {
		parent->has_tool = true;
		parent->current.tool = {};
	}

	void tool_handler::onElement(xml_config&, char const** attrs) {
		for (auto attr = attrs; *attr; attr += 2) {
			auto const name = std::string_view{attr[0]};
			if (name == "which"sv || name == "PATH"sv) {
				auto const value = std::u8string_view{
				    reinterpret_cast<char8_t const*>(attr[1])};
				parent->has_tool = true;
				parent->current.tool =
				    env::tool{false, {value.data(), value.size()}};
			}
		}
	}

	std::unique_ptr<handler_interface> command_handler_base::onChild(
	    std::u8string_view name) {
		if (has_tool) {
			processText();
			if (name == u8"var"sv) return std::make_unique<var_handler>(this);
			return {};
		}

		if (name == u8"cxx"sv) return std::make_unique<cxx_handler>(this);
		if (name == u8"tool"sv) return std::make_unique<tool_handler>(this);
		return {};
	}

	void command_handler_base::onCharacter(std::u8string_view data) {
		if (has_tool)
			text.append(reinterpret_cast<char const*>(data.data()),
			            data.size());
	}

	void command_handler_base::onStop(xml_config&) {
		if (!has_tool) return;
		processText();
		if (!current.args.empty()) {
			auto& back = current.args.back();
			if (std::holds_alternative<std::string>(back)) {
				auto& str = std::get<std::string>(back);
				if (!str.empty() && str.back() == ' ') str.pop_back();
				if (str.empty()) current.args.pop_back();
			}
		}
	}

	std::string spaced(std::string_view stripped,
	                   bool needs_pre,
	                   bool needs_post) {
		std::string result{};
		result.reserve(stripped.length() + (needs_pre ? 1 : 0) +
		               (needs_post ? 1 : 0));
		if (needs_pre) result.push_back(' ');
#if 1
		result.append(stripped);
#else
		bool in_space = false;
		for (auto c : stripped) {
			auto is_space = std::isspace(static_cast<unsigned char>(c));
			if (in_space) {
				if (is_space) continue;
				in_space = false;
			}
			if (!is_space) {
				result.push_back(c);
				continue;
			}
			in_space = true;
			result.push_back(' ');
		}
#endif
		if (needs_post) result.push_back(' ');
		return result;
	}

	void command_handler_base::processText() {
		auto view = std::string_view{text};
		auto const orig_length = view.length();
		view = lstrip_sv(view);
		auto const left_length = view.length();
		view = rstrip_sv(view);
		auto const right_length = view.length();
		current.args.push_back(spaced(view, left_length != orig_length,
		                              left_length != right_length));
		text.clear();
	}

	void command_handler::onStop(xml_config& cfg) {
		command_handler_base::onStop(cfg);
		if (has_tool) parent->commands.push_back(std::move(current));
	}

	void rule_handler::onElement(xml_config&, char const** attrs) {
		for (auto attr = attrs; *attr; attr += 2) {
			auto const name = std::string_view{attr[0]};
			if (name == "id"sv) rule_name.assign(attr[1]);
		}
	}

	std::unique_ptr<handler_interface> rule_handler::onChild(
	    std::u8string_view name) {
		if (name == u8"command"sv)
			return std::make_unique<command_handler>(this);
		return {};
	}

	void rule_handler::onStop(xml_config& cfg) {
		cfg.str_rules[std::move(rule_name)] = std::move(commands);
	}

	std::unique_ptr<handler_interface> rules_handler::onChild(
	    std::u8string_view name) {
		if (name == u8"rule"sv) return std::make_unique<rule_handler>();
		return {};
	}

	void include_dirs_handler::onElement(xml_config& cfg, char const** attrs) {
		for (auto attr = attrs; *attr; attr += 2) {
			auto const name = std::string_view{attr[0]};
			auto const value = std::string_view{attr[1]};
			if (name == "output"sv)
				cfg.out->include_dirs.output = value == "stdout"sv
				                                   ? include_dirs::use_stdout
				                                   : include_dirs::use_stderr;
			else if (name == "start"sv)
				cfg.out->include_dirs.filter_start.assign(value);
			else if (name == "stop"sv)
				cfg.out->include_dirs.filter_stop.assign(value);
		}
	}

	void include_dirs_handler::onStop(xml_config& cfg) {
		command_handler_base::onStop(cfg);
		if (has_tool) cfg.out->include_dirs.filter = std::move(current);
	}

	void bmi_cache_handler::onElement(xml_config& cfg, char const** attrs) {
		for (auto attr = attrs; *attr; attr += 2) {
			auto const name = std::string_view{attr[0]};
			auto const value =
			    std::u8string_view{reinterpret_cast<char8_t const*>(attr[1])};
			if (name == "dirname"sv)
				cfg.out->bmi_decl.dirname.assign(value);
			else if (name == "ext"sv)
				cfg.out->bmi_decl.ext.assign(value);
			else if (name == "type"sv)
				cfg.out->bmi_decl.type =
				    (value == u8"side-effect"sv ? bmi_decl::side_effect
				                                : bmi_decl::direct);
			else if (name == "partitions"sv)
				cfg.out->bmi_decl.supports_parition = boolVal(value);
		}
	}

	void ident_handler::onElement(xml_config& cfg, char const** attrs) {
		for (auto attr = attrs; *attr; attr += 2) {
			auto const name = std::string_view{attr[0]};
			auto const value =
			    std::u8string_view{reinterpret_cast<char8_t const*>(attr[1])};
			if (name == "compat"sv)
				cfg.out->ident.compat.assign(value);
			else if (name == "exe"sv)
				cfg.out->ident.exe.assign(value);
			else if (name == "name"sv)
				cfg.out->ident.name.assign(value);
			else if (name == "guard"sv)
				cfg.out->ident.guard.assign(value);
			else if (name == "version"sv)
				cfg.out->ident.version.assign(value);
			else if (name == "find-tripple"sv)
				cfg.out->ident.find_tripple = boolVal(value);
		}
	}

	std::unique_ptr<handler_interface> compiler_handler::onChild(
	    std::u8string_view name) {
		if (name == u8"ident"sv) return std::make_unique<ident_handler>();
		if (name == u8"bmi-cache"sv)
			return std::make_unique<bmi_cache_handler>();
		if (name == u8"include-dirs"sv)
			return std::make_unique<include_dirs_handler>();
		if (name == u8"rules"sv) return std::make_unique<rules_handler>();
		return {};
	}

	handler_interface::~handler_interface() = default;
	void handler_interface::onElement(xml_config&, char const**) {}
	std::unique_ptr<handler_interface> handler_interface::onChild(
	    std::u8string_view) {
		return {};
	}
	void handler_interface::onCharacter(std::u8string_view) {}
	void handler_interface::onStop(xml_config&) {}

	std::unique_ptr<handler_interface> document_handler::onChild(
	    std::u8string_view name) {
		if (name == u8"compiler"sv) return std::make_unique<compiler_handler>();
		return {};
	}
}  // namespace xml
