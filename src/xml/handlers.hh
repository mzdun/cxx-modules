#pragma once

#include <xml/types.hh>

using namespace std::literals;

namespace xml {
	struct xml_config {
		compiler_factory_config* out{};
		std::map<std::string, commands> str_rules{};
	};

	struct handler_interface {
		virtual ~handler_interface();
		virtual void onElement(xml_config&, char const** attrs);
		virtual std::unique_ptr<handler_interface> onChild(std::u8string_view);
		virtual void onCharacter(std::u8string_view);
		virtual void onStop(xml_config&);

		static bool boolVal(std::u8string_view value) noexcept {
			return (value != u8"false"sv) && (value != u8"no"sv) &&
			       (value != u8"0"sv);
		}
	};

	struct document_handler : handler_interface {
		std::unique_ptr<handler_interface> onChild(
		    std::u8string_view name) override;
	};
}  // namespace xml
