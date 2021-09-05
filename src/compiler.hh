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

struct compiler_id {
	std::string_view id;
	std::string_view if_macro;
	std::string_view version_macros;
};

struct compiler_factory {
	virtual ~compiler_factory();
	virtual std::unique_ptr<compiler> create(
	    std::u8string_view path,
	    std::string_view id,
	    std::string_view version) const = 0;
	virtual compiler_id get_compiler_id() const = 0;
};

template <typename Impl>
struct simple_compiler_factory : compiler_factory {
	explicit simple_compiler_factory(compiler_id const& id) : id_{id} {};
	std::unique_ptr<compiler> create(std::u8string_view path,
	                                 std::string_view id,
	                                 std::string_view version) const override {
		return std::make_unique<Impl>(path, id, version);
	};

	compiler_id get_compiler_id() const override { return id_; }

private:
	compiler_id id_{};
};

struct compiler_info {
	fs::path exec;
	std::string name;
	std::pair<std::string, std::string> id;
	compiler_factory const* factory{};
	static size_t register_impl(std::unique_ptr<compiler_factory>&&);
	static compiler_info from_environment();
	std::optional<std::string> preproc(fs::path const&) const;
	std::unique_ptr<compiler> create() const {
		if (!factory) return {};
		return factory->create(exec.generic_u8string(), id.first, id.second);
	}
};

template <typename Impl>
struct compiler_registrar {
	compiler_registrar(std::string_view id,
	                   std::string_view if_macro,
	                   std::string_view version_macros) {
		compiler_info::register_impl(
		    std::make_unique<simple_compiler_factory<Impl>>(
		        compiler_id{id, if_macro, version_macros}));
	}
};
