#pragma once
#include <base/compiler.hh>
#include <filesystem>
#include "types.hh"

namespace xml {
	struct factory : compiler_factory {
		factory(std::filesystem::path&& filename,
		        compiler_factory_config&& cfg);
		std::unique_ptr<::compiler> create(
		    logger& log,
		    std::u8string_view path,
		    std::string_view id,
		    std::string_view version) const override;
		compiler_id get_compiler_id() const override;

	private:
		std::filesystem::path filename;
		compiler_factory_config cfg;
	};

}  // namespace xml