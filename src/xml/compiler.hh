#pragma once

#include <base/compiler.hh>
#include <env/binary_interface.hh>
#include <env/command_list.hh>
#include <env/include_locator.hh>

namespace xml {
	struct compiler : ::compiler {
		compiler(env::include_locator&& includes,
		         env::binary_interface&& bin,
		         env::command_list&& commands)
		    : includes_{std::move(includes)}
		    , bin_{std::move(bin)}
		    , commands_{std::move(commands)} {}

		void mapout(build_info const& build, generator& gen) override;
		std::vector<templated_string> commands_for(rule_type) override;

	private:
		env::include_locator includes_;
		env::binary_interface bin_;
		env::command_list commands_;
	};
}  // namespace xml
