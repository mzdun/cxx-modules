#pragma once

#include "env/path.hh"

namespace env {
	class command_list {
	public:
		template <typename, rule_type... rules>
		struct rule_counter_impl {
			static constexpr size_t count = sizeof...(rules);
		};
#define NAME(X) , rule_type::X
		using rule_counter = rule_counter_impl<void RULE(NAME)>;
#undef NAME
		static constexpr size_t rule_count = rule_counter::count;

		using commands = std::vector<templated_string>;

		command_list(
		    env::paths const& paths,
		    std::map<rule_type, std::vector<env::command>> const& edges);

		commands const& get(rule_type rule) const noexcept;

	private:
		std::vector<commands> commands_{rule_count, commands{}};
	};
}  // namespace env
