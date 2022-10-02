module name;
import <iostream>;
import <string>;

namespace ns {
	void greet(std::string const& name) {
		std::cout << "Hello, " << name << "!\n";
	}
}  // namespace ns
