#include <base/compiler.hh>

using namespace std::literals;

struct cl : compiler {
	cl(std::u8string_view, std::string_view, std::string_view) {}
};

static compiler_registrar<cl> register_cl{
    "MSVC"sv,
    "defined(_MSC_VER)"sv,
    "_MSC_VER"sv,
};
