#include <base/compiler.hh>
#include <base/logger.hh>
#include <base/types.hh>
#include <base/utils.hh>
#include <base/xml.hh>
#include <generators/dot.hh>
#include <generators/ninja.hh>
#include <iostream>

#ifdef __has_include
#if __has_include(<generators/vs.hh>)
#include <generators/vs.hh>
#endif
#endif

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

	load_xml_compilers();

	auto source_dir = fs::current_path();
	auto binary_dir = source_dir / u8"build"sv;

	std::error_code ec{};
	fs::create_directories(binary_dir, ec);
	if (ec) {
		std::cerr << "c++modules: cannot create "
		          << as_sv(binary_dir.generic_u8string()) << ": "
		          << ec.message() << '\n';
		return 1;
	}

	auto const comp = compiler_info::from_environment(binary_dir);
	auto const build = build_info::analyze(project::load(source_dir), comp,
	                                       source_dir, binary_dir);

	logger log{build, comp};
	log.print();

	using PlatformGenerator = ninja;  // FUTURE: ninja || vstudio

	PlatformGenerator gen{};
	if (auto cxx = comp.create(log); cxx) cxx->mapout(build, gen);

	auto back_to_sources = build.source_from_binary();
	gen.generate(back_to_sources, build.binary_dir);
	std::move(gen).to<dot>().generate(back_to_sources, build.binary_dir);
}
