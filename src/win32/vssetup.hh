#pragma once
#include <filesystem>

namespace vssetup {
	std::filesystem::path find_compiler();
	std::string win10sdk();
}