#pragma once

#include <xml/handlers.hh>

namespace xml {
	struct command_handler_base;
	struct rule_handler;

	struct var_handler : handler_interface {
		command_handler_base* parent{};

		var_handler(command_handler_base* parent) : parent{parent} {}
		void onElement(xml_config&, char const** attrs) override;
	};

	struct cxx_handler : handler_interface {
		command_handler_base* parent{};

		cxx_handler(command_handler_base* parent) : parent{parent} {}
		void onElement(xml_config&, char const** attrs) override;
	};

	struct tool_handler : handler_interface {
		command_handler_base* parent{};

		tool_handler(command_handler_base* parent) : parent{parent} {}
		void onElement(xml_config&, char const** attrs) override;
	};

	struct command_handler_base : handler_interface {
		env::command current{};
		std::string text{};
		bool has_tool{false};

		std::unique_ptr<handler_interface> onChild(
		    std::u8string_view name) override;
		void onCharacter(std::u8string_view data) override;
		void onStop(xml_config&) override;
		void processText();
	};

	struct command_handler : command_handler_base {
		command_handler(rule_handler* parent) : parent{parent} {}
		rule_handler* parent;

		void onStop(xml_config& cfg) override;
	};

	struct rule_handler : handler_interface {
		std::string rule_name;
		xml::commands commands;

		void onElement(xml_config&, char const** attrs) override;
		std::unique_ptr<handler_interface> onChild(
		    std::u8string_view name) override;
		void onStop(xml_config& cfg) override;
	};

	struct rules_handler : handler_interface {
		std::unique_ptr<handler_interface> onChild(
		    std::u8string_view name) override;
	};

	struct include_dirs_handler : command_handler_base {
		void onElement(xml_config& cfg, char const** attrs) override;
		void onStop(xml_config& cfg) override;
	};

	struct bmi_cache_handler : handler_interface {
		void onElement(xml_config& cfg, char const** attrs) override;
	};

	struct ident_handler : handler_interface {
		void onElement(xml_config& cfg, char const** attrs) override;
	};

	struct compiler_handler : handler_interface {
		std::unique_ptr<handler_interface> onChild(
		    std::u8string_view name) override;
	};
}  // namespace xml
