#pragma once

#include <filesystem>
#include <optional>
#include "xml/types.hh"

namespace xml {
	bool parse(std::filesystem::path const& filename, compiler_factory_config&);
}