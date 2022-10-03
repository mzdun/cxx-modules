import <iostream>;
import caps;

int main() {
	auto const capabilites = caps::load_config();
	std::cout << capabilites;
}
