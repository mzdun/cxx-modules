#include "env/command_list.hh"
#include <base/utils.hh>
#include <algorithm>

namespace env {
	namespace {
		templated_string simplify(env::paths const& paths,
		                          env::command const& cmd) {
			templated_string result{};
			result.reserve(cmd.args.size() + 1);
			result.push_back(as_str(paths.which(cmd.tool)));
			result.insert(result.end(), cmd.args.begin(), cmd.args.end());
			return result;
		}
		command_list::commands simplify(env::paths const& paths,
		                                std::vector<env::command> const& edge) {
			command_list::commands result{};
			result.reserve(edge.size());
			std::transform(
			    edge.begin(), edge.end(), std::back_inserter(result),
			    [&](auto const& cmd) { return simplify(paths, cmd); });
			return result;
		}
	}  // namespace

	command_list::command_list(
	    env::paths const& paths,
	    std::map<rule_type, std::vector<env::command>> const& edges) {
		for (auto const& [rule, cmds] : edges) {
			auto const index = static_cast<size_t>(rule);
			if (index >= commands_.size()) {
				continue;
			}
			commands_[index] = simplify(paths, cmds);
		}
	}

	command_list::commands const& command_list::get(
	    rule_type rule) const noexcept {
		static commands empty{};
		auto const index = static_cast<size_t>(rule);
		if (index >= commands_.size()) return empty;

		return commands_[index];
	}
}  // namespace env
