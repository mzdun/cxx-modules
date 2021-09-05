#pragma once

#include <string_view>

using namespace std::literals;

// clang-format off
struct version_t {
	unsigned major{@PROJECT_VERSION_MAJOR@};
    unsigned minor{@PROJECT_VERSION_MINOR@};
    unsigned patch{@PROJECT_VERSION_PATCH@};
    std::string_view stability{"@PROJECT_VERSION_STABILITY@"sv};
};
// clang-format on

constexpr version_t version{};

constexpr auto install_dir = "@CMAKE_INSTALL_PREFIX@"sv;
constexpr auto share_dir = "@CMAKE_INSTALL_PREFIX@/@SHAREDIR@"sv;

#ifndef NDEBUG
constexpr auto project_dir = "@CMAKE_CURRENT_SOURCE_DIR@"sv;
#endif
