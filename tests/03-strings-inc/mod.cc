module;
#include <string>
export module name;

using namespace std::literals;

namespace ns {
	export std::string get_name() { return "World [strings/include]"s; }
}  // namespace ns
