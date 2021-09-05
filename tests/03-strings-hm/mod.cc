export module name;
import<string>;

using namespace std::literals;

namespace ns {
	export std::string get_name() { return "World [strings/header-module]"s; }
}  // namespace ns
