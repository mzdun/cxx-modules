#pragma once

#include <base/generator.hh>

class msbuild : public generator {
public:
	void generate(std::filesystem::path const&,
	              std::filesystem::path const&) override;

	std::u8string filename(std::filesystem::path const& back_to_sources,
	                       artifact const&);
};
