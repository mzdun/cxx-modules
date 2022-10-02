export module name;
import <string>;

namespace ns {
	export void greet(std::string const& name);

	export std::string kind() {
		return "static-lib";
	}
}  // namespace ns
