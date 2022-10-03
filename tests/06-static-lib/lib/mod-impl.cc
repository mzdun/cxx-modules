module name;
import <iostream>;
import <string>;

namespace ns {
	extern std::string internal();

	void greet(std::string const& name) {
		std::cout << "Hello, " << name << "! (" << internal() << ")\n";
	}
}  // namespace ns
