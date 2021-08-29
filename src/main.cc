#include <iostream>
#include "compiler.hh"
#include "generators/dot.hh"
#include "generators/ninja.hh"
#include "logger.hh"
#include "scanner.hh"
#include "types.hh"
#include "utils.hh"

using namespace std::literals;

int main(int argc, char** argv) {
	if (argc > 1) {
		std::error_code ec{};
		fs::current_path(argv[1], ec);
		if (ec) {
			std::cerr << "c++modules: cannot change directory to " << argv[1]
			          << ": " << ec.message() << '\n';
			return 1;
		}
	}

	auto source_dir = fs::current_path();
	auto build_dir = source_dir / u8"build"sv;

	auto const comp = compiler_info::from_environment();
	auto const build = build_info::analyze(project::load(source_dir), comp,
	                                       source_dir, build_dir);

	std::error_code ec{};
	fs::create_directories(build.build_dir, ec);
	if (ec) {
		std::cerr << "c++modules: cannot create " << as_sv(build.build_dir)
		          << ": " << ec.message() << '\n';
		return 1;
	}

	logger::print(build, comp);

	using PlatformGenerator = ninja;  // FUTURE: ninja || vstudio

	PlatformGenerator gen{};
	if (auto cxx = comp.create(); cxx) cxx->mapout(build, gen);

	auto back_to_sources = build.source_from_build();
	gen.generate(back_to_sources, build.build_dir);
	std::move(gen).to<dot>().generate(back_to_sources, build.build_dir);
}
