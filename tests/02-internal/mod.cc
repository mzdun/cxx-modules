module;
#include <iostream>
#include <string>

export module name;

namespace ns {
	std::string get_name() { return "Y'all [internal]"; }
	export void greet() { std::cout << "Hello, " << get_name() << "!\n"; }
}  // namespace ns
