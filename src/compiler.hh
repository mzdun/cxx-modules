#pragma once

#include <fs/file.hh>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include "generator.hh"

struct compiler {
	virtual ~compiler();
	virtual void mapout(struct build_info const&, generator&);
	virtual std::vector<templated_string> commands_for(rule_type);

protected:
	inline auto bit(rule_type rule) {
		return 1ull << static_cast<std::underlying_type_t<rule_type>>(rule);
	}

	std::map<std::u8string, size_t> register_projects(struct build_info const&,
	                                                  generator&);
	target create_project_target(struct build_info const&,
	                             struct project const&,
	                             struct project_info const&,
	                             std::map<std::u8string, size_t> const&);
	void add_rules(unsigned long long bits, generator&);
	size_t get_setup_id(std::u8string const& name,
	                    std::map<std::u8string, size_t> const& ids) {
		auto it = ids.find(name);
		if (it != ids.end()) return it->second;
		return std::numeric_limits<size_t>::max();
	};

	static fs::path where(fs::path const& toolname, bool real_paths);
};

struct compiler_factory {
	using create_type = std::unique_ptr<compiler> (*)(std::u8string_view path,
	                                                  std::string_view id,
	                                                  std::string_view version);

	std::string_view id;
	std::string_view if_macro;
	std::string_view version_macros;
	create_type create{};
};

struct compiler_info {
	fs::path exec;
	std::string name;
	std::pair<std::string, std::string> id;
	compiler_factory const* factory{};
	static size_t register_impl(compiler_factory const&);
	static compiler_info from_environment();
	std::optional<std::string> preproc(fs::path const&) const;
	std::unique_ptr<compiler> create() const {
		if (!factory) return {};
		return factory->create(exec.generic_u8string(), id.first, id.second);
	}
};

template <typename Impl>
struct compiler_registrar {
	static std::unique_ptr<compiler> create(std::u8string_view path,
	                                        std::string_view id,
	                                        std::string_view version) noexcept {
		return std::make_unique<Impl>(path, id, version);
	};

	compiler_registrar(std::string_view id,
	                   std::string_view if_macro,
	                   std::string_view version_macros) {
		compiler_info::register_impl(
		    {id, if_macro, version_macros, compiler_registrar<Impl>::create});
	}
};
