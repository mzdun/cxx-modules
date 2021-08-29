#pragma once

#include <fstream>
#include "compiler.hh"
#include "types.hh"

struct logger {
	build_info const& build;
	compiler_info const& cxx;
	fs::path logname{fs::path{build.build_dir} / "c++modules.txt"};
	std::ofstream output{logname};

	static void print(build_info const& build, compiler_info const& cxx) {
		logger{build, cxx}.print();
	}

private:
	void print();

	void compiler();
	void config();
	void projects();
	void source_refs();
	void modules();
};
