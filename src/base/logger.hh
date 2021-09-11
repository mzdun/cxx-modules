#pragma once

#include <base/compiler.hh>
#include <fstream>
#include "types.hh"

struct logger {
	build_info const& build;
	compiler_info const& cxx;
	fs::path logname{fs::path{build.build_dir} / "c++modules.txt"};
	std::ofstream output{logname};

	void print();

private:
	void compiler();
	void config();
	void projects();
	void source_refs();
	void modules();
};
