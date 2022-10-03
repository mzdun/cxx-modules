import <string>;
#if __has_include(<format>)
import <format>;
#endif

import name;

int main() {
#if __has_include(<format>)
	ns::greet(std::format("Y'all [{} + {{fmt}}]", ns::kind()));
#else
	ns::greet("Y'all [" + ns::kind() + "]");
#endif
}
