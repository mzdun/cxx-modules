module;
#include <iostream>
#include <string>

module name;

namespace ns {
	void greet(std::string const& name) {
		std::cout << "Hello, " << name << "!\n";
	}
}  // namespace ns
