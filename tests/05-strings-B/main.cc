#include <iostream>
#include <string>

import name;

void greet(std::string const& name) {
	std::cout << "Hello, " << name << "!\n";
}

int main() {
	greet(ns::get_name());
}
