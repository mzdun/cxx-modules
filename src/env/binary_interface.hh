#pragma once

#include <base/generator.hh>
#include <base/types.hh>
#include <base/utils.hh>
#include <env/include_locator.hh>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

namespace env {
	class binary_interface {
	public:
		binary_interface(bool supports_paritions,
		                 bool standalone_interface,
		                 std::filesystem::path const& dirname,
		                 std::filesystem::path const& ext);

		friend std::ostream& operator<<(std::ostream& out,
		                                binary_interface const& biin) {
			return out << as_sv(biin.dirname_) << '*' << as_sv(biin.ext_)
			           << "\n    type: "
			           << (biin.standalone_interface_ ? "direct"
			                                          : "side-effect")
			           << "\n    supports paritions: "
			           << (biin.partition_separator_ == '-' ? "yes" : "no")
			           << '\n';
		}

		bool standalone_interface() const noexcept {
			return standalone_interface_;
		}
		std::u8string as_interface(mod_name const& name);
		std::optional<artifact> from_module(
		    include_locator& locator,
		    std::filesystem::path const& source_path,
		    mod_name const& ref);
		void add_targets(std::vector<target>& targets,
		                 rule_types& rules_needed) const;

	private:
		std::optional<artifact> header_module(
		    include_locator& locator,
		    std::filesystem::path const& source_path,
		    mod_name const& ref);

		char8_t partition_separator_;
		bool standalone_interface_;
		std::u8string dirname_;
		std::u8string ext_;

		std::map<std::u8string,
		         std::tuple<std::u8string, std::u8string, std::u8string>>
		    header_modules_{};
	};
}  // namespace env
