#include "xml/factory.hh"
#include <charconv>
#include "env/binary_interface.hh"
#include "env/command_list.hh"
#include "env/include_locator.hh"
#include "env/path.hh"
#include "logger.hh"

namespace xml {
	namespace {
		unsigned get_version_major(std::string_view version) {
			auto const major_sv = version.substr(0, version.find(' '));

			auto const end = major_sv.data() + major_sv.size();
			unsigned major{};
			auto result = std::from_chars(major_sv.data(), end, major);
			if (result.ec != std::errc{} || result.ptr != end) major = 255;

			return major;
		}
	}  // namespace

	factory::factory(std::filesystem::path&& filename,
	                 compiler_factory_config&& cfg)
	    : filename{std::move(filename)}, cfg{std::move(cfg)} {}

	std::unique_ptr<compiler> factory::create(logger& log,
	                                          std::u8string_view path,
	                                          std::string_view id,
	                                          std::string_view version) const {
		auto const paths =
		    env::paths::parser{path, cfg.ident.exe, get_version_major(version)}
		        .find();

		env::include_locator locator{
		    paths,
		    cfg.include_dirs.output == include_dirs::use_stdout,
		    cfg.include_dirs.filter_start,
		    cfg.include_dirs.filter_stop,
		    cfg.include_dirs.filter,
		};

		env::binary_interface biin{cfg.bmi_decl.supports_parition,
		                           cfg.bmi_decl.type == bmi_decl::direct,
		                           cfg.bmi_decl.dirname, cfg.bmi_decl.ext};

		env::command_list cmd_list{paths, cfg.rules};

		log.output << "\n------------------------------------\n\n"sv;

		log.output << "from: " << filename.generic_string() << '\n';
		log.output << "path info: " << as_sv(paths.root.generic_u8string())
		           << " :: " << as_sv(paths.triple)
		           << " :: " << as_sv(cfg.ident.exe)
		           << " :: " << as_sv(paths.suffix) << '\n'
		           << biin << std::flush;

		return {};
	}

	compiler_id factory::get_compiler_id() const {
		return {
		    as_sv(cfg.ident.name),
		    as_sv(cfg.ident.guard),
		    as_sv(cfg.ident.version),
		};
	}
}  // namespace xml