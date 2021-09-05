#pragma once

#include <filesystem>
#include <string>
#include <variant>
#include <vector>
#include "types.hh"

#define VAR(X)      \
	X(INPUT)        \
	X(OUTPUT)       \
	X(MAIN_OUTPUT)  \
	X(LINK_FLAGS)   \
	X(LINK_PATH)    \
	X(LINK_LIBRARY) \
	X(DEFINES)      \
	X(CFLAGS)       \
	X(CXXFLAGS)

enum class var {
#define ENUM(NAME) NAME,
	VAR(ENUM)
#undef ENUM
};

struct named_var {
	std::string value;
};
using templated_string = std::vector<std::variant<std::string, named_var, var>>;

#define RULE(X)     \
	X(MKDIR)        \
	X(COMPILE)      \
	X(EMIT_BMI)     \
	X(EMIT_INCLUDE) \
	X(LINK_STATIC)  \
	X(LINK_SO)      \
	X(LINK_MOD)     \
	X(LINK_EXECUTABLE)

enum class rule_type {
#define ENUM(NAME) NAME,
	RULE(ENUM)
#undef ENUM
	    ARCHIVE = LINK_STATIC
};

struct rule_types {
	unsigned long long bits{};

	void set(rule_type rule) noexcept { bits |= bit(rule); }
	bool has(rule_type rule) const noexcept {
		auto const test = bit(rule);
		return (bits & test) == test;
	}

	static unsigned long long bit(rule_type rule) {
		return 1ull << static_cast<std::underlying_type_t<rule_type>>(rule);
	}
};

using rule_name = std::variant<std::monostate, std::string, rule_type>;

struct rule {
	rule_name name{};
	std::vector<templated_string> commands{};
	templated_string message{};

	static templated_string default_message(rule_type type);
};

struct file_ref {
	enum kind { input, output, linked, header_module, include };
	size_t prj;
	std::u8string path;
	kind type{output};
	std::u8string node_name{};
	auto operator<=>(file_ref const&) const = default;
};

struct mod_ref {
	mod_name mod;
	std::u8string path;
	auto operator<=>(mod_ref const&) const = default;
};

using artifact = std::variant<file_ref, mod_ref>;

struct filelist {
	std::vector<artifact> expl{};
	std::vector<artifact> impl{};
	std::vector<artifact> order{};
};

struct target {
	rule_name rule{};
	artifact main_output{};
	filelist inputs{};
	filelist outputs{};
	std::u8string edge{};
};

struct project_setup {
	std::u8string name;
	std::u8string objdir;
	std::u8string subdir;
};

struct generator {
	generator() = default;
	generator(generator const&) = default;
	generator(generator&) = default;
	virtual ~generator();
	void set_rules(std::vector<rule> const& rules) { rules_ = rules; }
	void set_rules(std::vector<rule>&& rules) { rules_ = std::move(rules); }

	void set_setups(std::vector<project_setup> const& setups) {
		setups_ = setups;
	}
	void set_setups(std::vector<project_setup>&& setups) {
		setups_ = std::move(setups);
	}
	size_t register_setup(project_setup&& setup) {
		auto const result = setups_.size();
		setups_.push_back(std::move(setup));
		return result;
	}

	void set_targets(std::vector<target> const& targets) { targets_ = targets; }
	void set_targets(std::vector<target>&& targets) {
		targets_ = std::move(targets);
	}

	template <typename Gen>
	Gen copyTo() const& {
		Gen result{};
		result.set_rules(rules_);
		result.set_setups(setups_);
		result.set_targets(targets_);
		return result;
	}

	template <typename Gen>
	Gen to() && {
		Gen result{};
		result.set_rules(std::move(rules_));
		result.set_setups(std::move(setups_));
		result.set_targets(std::move(targets_));
		return result;
	}

	virtual void generate(std::filesystem::path const&,
	                      std::filesystem::path const&) = 0;

protected:
	std::vector<rule> rules_;
	std::vector<project_setup> setups_;
	std::vector<target> targets_;
};