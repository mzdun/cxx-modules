#pragma once

#include <filesystem>
#include <string>
#include <variant>
#include <vector>
#include "types.hh"

enum class var {
	INPUT,
	OUTPUT,
	MAIN_OUTPUT,
	LINK_FLAGS,
	LINK_PATH,
	LINK_LIBRARY,
	DEFINES,
	CFLAGS,
	CXXFLAGS
};

using templated_string = std::vector<std::variant<std::string, var>>;

enum class rule_type {
	MKDIR,
	COMPILE,
	EMIT_BMI,
	ARCHIVE,
	LINK_SO,
	LINK_MOD,
	LINK_EXECUTABLE
};

using rule_name = std::variant<std::monostate, std::string, rule_type>;

struct rule {
	rule_name name{};
	std::vector<templated_string> commands{};
	templated_string message{};

	static templated_string default_message(rule_type type);
};

struct file_ref {
	enum kind { input, output, linked };
	size_t prj;
	std::u8string path;
	kind type{output};
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