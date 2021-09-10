#pragma once

#include <base/generator.hh>
#include <base/utils.hh>
#include "env/path.hh"

namespace xml {
	struct ident {
		std::u8string compat{};
		std::u8string exe{};
		std::u8string name{};
		std::u8string guard{};
		std::u8string version{};
		bool find_tripple{false};
	};

	struct bmi_decl {
		enum kind { direct, side_effect };
		std::u8string dirname{};
		std::u8string ext{};
		kind type{direct};
		bool supports_parition{true};
	};

	struct include_dirs {
		enum { use_stderr, use_stdout } output{use_stderr};
		std::string filter_start{};
		std::string filter_stop{};
		env::command filter;
	};

	using commands = std::vector<env::command>;
	struct compiler_factory_config {
		xml::ident ident{};
		xml::bmi_decl bmi_decl{};
		xml::include_dirs include_dirs{};
		std::map<rule_type, commands> rules{};
	};
}  // namespace xml