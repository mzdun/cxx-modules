#include <iostream>
#include <string>

void greet(std::string const& name) {
	std::cout << "Hello, " << name << "!\n";
}

int main() {
	greet("Y'all [cxx17]");
}
