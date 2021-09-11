#include "logger.hh"
#include <base/utils.hh>
#include <iostream>

void logger::print() {
	if (!output) {
		std::cerr << "c++modules: error: cannot open "
		          << as_sv(logname.generic_u8string()) << '\n';
		return;
	}

	static constexpr auto separator =
	    "\n------------------------------------\n\n"sv;

	compiler();
	output << separator;
	config();
	output << separator;
	projects();
	output << separator;
	source_refs();
	output << separator;
	modules();
	output.flush();
}

void logger::compiler() {
	output << "compiler: " << cxx.id.first << '/' << cxx.id.second
	       << "\n"
	          "executable: "
	       << as_sv(cxx.exec.generic_u8string()) << '\n';
}

void logger::config() {
	output << "source dir: " << as_sv(build.source_dir)
	       << "\n"
	          "binary dir: "
	       << as_sv(build.binary_dir) << '\n';
	if (!build.imports.empty()) output << "requires\n";
	for (auto const& [key, imports] : build.imports) {
		output << "    " << as_sv(key) << '\n';
		for (auto const& mod : imports)
			output << "        " << as_sv(mod.toString()) << '\n';
	}
	if (!build.exports.empty()) output << "provides\n";
	for (auto const& [key, mod] : build.exports) {
		output << "    " << as_sv(key) << " -> " << as_sv(mod.toString())
		       << '\n';
	}
}

void logger::projects() {
	for (auto const& [prj, info] : build.projects) {
		output << as_sv(project::kind2str[prj.type]) << " " << as_sv(prj.name)
		       << ' ' << info.subdir << '\n';
		if (!info.imports.empty()) output << "    requires\n";
		for (auto const& mod : info.imports)
			output << "        " << as_sv(mod.toString()) << '\n';
		if (!info.exports.empty()) output << "    provides\n";
		for (auto const& mod : info.exports)
			output << "        " << as_sv(mod.toString()) << '\n';
		if (!info.sources.empty()) output << "    includes\n";
		for (auto const& source : info.sources)
			output << "        " << as_sv(source) << '\n';
		if (!info.links.empty()) output << "    links to\n";
		for (auto const& linked : info.links)
			output << "        " << as_sv(linked.filename()) << '\n';
	}
}

void logger::source_refs() {
	std::set<std::u8string> names;

	for (auto const& [path, _] : build.exports)
		names.insert(path);
	for (auto const& [path, _] : build.imports)
		names.insert(path);

	for (auto const& path : names) {
		output << as_sv(path);

		auto mod_it = build.exports.find(path);
		if (mod_it != build.exports.end())
			output << " -> " << as_sv(mod_it->second.toString());
		output << '\n';

		auto mods_it = build.imports.find(path);
		if (mods_it != build.imports.end()) {
			for (auto const& mod : mods_it->second)
				output << "    " << as_sv(mod.toString()) << '\n';
		}
	}
}

void logger::modules() {
	for (auto const& [modname, module] : build.modules) {
		auto const mod = modname.toString();
		if (mod.empty())
			output << "<global module>\n";
		else
			output << as_sv(mod) << " -> " << as_sv(module.interface) << '\n';
		if (!module.sources.empty()) output << "    sources\n";
		for (auto const& source : module.sources)
			output << "        " << as_sv(source) << '\n';
		if (!module.req.empty()) output << "    requires\n";
		for (auto const& name : module.req)
			output << "        " << as_sv(name.toString()) << '\n';
		if (!module.libs.empty()) output << "    linker\n";
		for (auto const& lib : module.libs)
			output << "        " << as_sv(lib.filename()) << '\n';
	}
}
