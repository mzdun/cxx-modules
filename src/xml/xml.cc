#include "xml/compiler.hh"
#include <iostream>
#include <map>
#include "dirs.hh"
#include "xml/factory.hh"
#include "xml/parser.hh"

namespace {
	void register_xml_compiler(std::filesystem::path&& filename,
	                           xml::compiler_factory_config&& cfg) {
		compiler_info::register_impl(std::make_unique<xml::factory>(
		    std::move(filename), std::move(cfg)));
	}

	struct xml_compiler_info {
		fs::file_time_type mtime;
		std::filesystem::path filename;
		xml::compiler_factory_config cfg;
	};

	struct compiler_cache {
		std::map<std::u8string, xml_compiler_info> configs{};

		void add_config(fs::file_time_type mtime,
		                std::filesystem::path const& filename,
		                xml::compiler_factory_config&& cfg) {
			auto it = configs.lower_bound(cfg.ident.name);

			if (it == configs.end() || it->first != cfg.ident.name) {
				auto key = cfg.ident.name;
				configs.insert(it, {
				                       std::move(key),
				                       {mtime, filename, std::move(cfg)},
				                   });
				return;
			}

			if (it->second.mtime < mtime) {
				it->second = {mtime, filename, std::move(cfg)};
			}
		}
	};
}  // namespace

void load_xml_compilers() {
	static constexpr auto comp_dir = "compilers"sv;

	compiler_cache cache{};

	for (auto const& dir : {
	         fs::path{install_dir} / share_dir / comp_dir,
#ifndef NDEBUG
	         fs::path{build_dir} / share_dir / comp_dir,
	         fs::path{project_dir} / "data"sv / comp_dir,
#endif
	     }) {
		std::error_code ec{};
		fs::directory_iterator reader{dir, ec};
		if (ec) continue;

		for (auto const& entry : reader) {
			if (!entry.is_regular_file()) continue;

			auto mtime = entry.last_write_time(ec);
			if (ec) {
				ec = std::error_code{};
				continue;
			}

			xml::compiler_factory_config cfg{};
			if (!xml::parse(entry.path(), cfg)) continue;

			cache.add_config(mtime, entry.path(), std::move(cfg));
		}
	};

	for (auto& [_, info] : cache.configs) {
		register_xml_compiler(std::move(info.filename), std::move(info.cfg));
	}
}