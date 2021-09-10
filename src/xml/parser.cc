#include "xml/parser.hh"
#include <base/compiler.hh>
#include <fs/file.hh>
#include <iostream>
#include <stack>
#include <xml/expat.hh>
#include <xml/handlers.hh>
#include "dirs.hh"

namespace xml {
	env::command vars_from(env::command&& xml_command) {
		env::command result{};
		result.tool = std::move(xml_command.tool);
		result.args.reserve(xml_command.args.size());
		for (auto& arg : xml_command.args) {
			if (std::holds_alternative<std::string>(arg))
				result.args.push_back(std::get<std::string>(arg));
			else if (std::holds_alternative<var>(arg))
				result.args.push_back(std::get<var>(arg));
			else {
				auto& name = std::get<named_var>(arg).value;
#define CASE(NAME)                        \
	if (name == #NAME##sv) {              \
		result.args.push_back(var::NAME); \
	} else
				VAR(CASE)
				result.args.push_back(named_var{std::move(name)});
#undef CASE
			}
		}
		return result;
	}

	commands vars_from(commands&& xml_commands) {
		commands result{};
		result.reserve(xml_commands.size());
		for (auto& command : xml_commands)
			result.push_back(vars_from(std::move(command)));
		return result;
	}

	std::optional<rule_type> rule_from(std::string_view name) {
#define CASE(NAME) \
	if (name == #NAME##sv) return rule_type::NAME;
		RULE(CASE)
#undef CASE
		return std::nullopt;
	}

	std::optional<std::map<rule_type, commands>> rules_from(
	    std::map<std::string, commands>&& rules) {
		std::optional<std::map<rule_type, commands>> result{
		    std::map<rule_type, commands>{}};

		auto& res = *result;

		for (auto& [key, value] : rules) {
			auto const type = rule_from(key);

			if (!type) {
				std::cerr << "error: unknown rule: " << key << '\n';
				result = std::nullopt;
				return result;
			}

			res[*type] = vars_from(std::move(value));
		}

		static constexpr rule_type rule_ids[] = {
#define NAME(X) rule_type::X,
		    RULE(NAME)
#undef NAME
		};

		for (auto const rule : rule_ids) {
			auto it = res.lower_bound(rule);
			if (it == res.end() || it->first != rule) {
				std::cerr << "warning: rule for ";
#define CASE(NAME)          \
	case rule_type::NAME:   \
		std::cerr << #NAME; \
		break;
				switch (rule) {
					RULE(CASE)
					default:
						std::cerr << "?[" << int(rule) << ']';
				};
#undef CASE
				std::cerr << " missing\n";
				res.insert(it, {rule, commands{}});
			}
		}

		return result;
	}

	class parser : public xml::ExpatBase<parser> {
		std::stack<std::unique_ptr<handler_interface>> handlers_{};
		int ignore_depth_{0};

		xml_config config_{};

	public:
		parser() { handlers_.push(std::make_unique<document_handler>()); }

		static bool load(std::filesystem::path const& filename,
		                 compiler_factory_config& output) {
			auto file = fs::fopen(filename);
			if (!file) return false;

			xml::parser ldr{};
			if (!ldr.create()) return false;
			ldr.enableElementHandler();
			ldr.enableCdataSectionHandler();
			ldr.enableCharacterDataHandler();

			ldr.config_.out = &output;

			char buffer[8196];
			while (auto const byte_count = file.load(buffer, sizeof(buffer))) {
				ldr.parse(buffer, static_cast<int>(byte_count), false);
			}
			ldr.parse(buffer, 0, true);

			if (output.ident.compat != u8"gcc"sv) return false;

			auto rules = xml::rules_from(std::move(ldr.config_.str_rules));
			if (!rules) return false;

			output.rules = std::move(*rules);
			return true;
		}

		void onStartElement(char const* name, char const** attrs) {
			if (!ignore_depth_) {
				auto const tag =
				    std::u8string_view{reinterpret_cast<char8_t const*>(name)};
				auto current = handlers_.top().get();
				auto next = current->onChild(tag);
				if (next) {
					next->onElement(config_, attrs);
					handlers_.push(std::move(next));
					return;
				}
			}
			std::cerr << '[' << ignore_depth_ << "] >>> " << name;
			for (auto attr = attrs; *attr; attr += 2) {
				std::cerr << ' ' << attr[0] << "=\"" << attr[1] << '"';
			}
			std::cerr << '\n';

			++ignore_depth_;
		}

		void onEndElement(char const* name) {
			if (ignore_depth_) {
				--ignore_depth_;
				std::cerr << '[' << ignore_depth_ << "] <<< " << name << '\n';
				return;
			}

			auto current = std::move(handlers_.top());
			handlers_.pop();
			current->onStop(config_);
		}

		void onCharacterData(char const* data, int length) {
			if (ignore_depth_) return;
			auto current = handlers_.top().get();
			current->onCharacter(
			    std::u8string_view{reinterpret_cast<char8_t const*>(data),
			                       static_cast<size_t>(length)});
		}
	};

	bool parse(std::filesystem::path const& filename,
	           compiler_factory_config& output) {
		return parser::load(filename, output);
	}
}  // namespace xml
