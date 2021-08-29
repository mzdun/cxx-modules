#pragma once

#include "../generator.hh"

class dot : public generator {
public:
	using generator::generator;
	void generate(std::filesystem::path const&,
	              std::filesystem::path const&) override;
};
