module caps;
import :config;
import <string>;

#ifdef _WIN64
#define OS "win32x64"s
#elif defined(_WIN32)
#define OS "win32x32"s
#endif

using namespace std::literals;

namespace caps {
	Config load_config() {
		return {.items = {
		            {"key"s, "value"s},
		            {"name"s, "contents"s},
		            {"os"s, OS},
		        }};
	}
}  // namespace caps
