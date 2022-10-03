export module caps : config;

import <string>;
import <map>;
import <iostream>;

export namespace caps {
	struct Config {
		std::map<std::string, std::string> items;

        friend std::ostream& operator<< (std::ostream& out, Config const& config) {
            for (auto const& [key, value] : config.items) {
                out << key << " = " << value << '\n';
            }
            return out;
        }
	};
}
