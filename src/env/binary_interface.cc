#include "env/binary_interface.hh"

namespace env {
	namespace {
		std::u8string append(char8_t ch, std::filesystem::path const& dirname) {
			auto result = dirname.generic_u8string();
			if (!result.empty() && result.back() != ch) result.push_back(ch);
			return result;
		}

		std::u8string prepend(char8_t ch, std::filesystem::path const& ext) {
			auto result = ext.generic_u8string();
			if (!result.empty() && result.front() != ch)
				result.insert(result.begin(), ch);
			return result;
		}
	}  // namespace

	binary_interface::binary_interface(bool supports_paritions,
	                                   bool standalone_interface,
	                                   std::filesystem::path const& dirname,
	                                   std::filesystem::path const& ext)
	    : partition_separator_{supports_paritions ? u8'-' : u8'.'}
	    , standalone_interface_{standalone_interface}
	    , dirname_{append(u8'/', dirname)}
	    , ext_{prepend(u8'.', ext)} {}

	std::u8string binary_interface::as_interface(mod_name const& name) {
		std::u8string fname{};
		fname.reserve(dirname_.size() + name.module.size() +
		              (name.part.empty() ? 0 : 1 + name.part.size()) +
		              ext_.size());
		fname.append(dirname_);
		fname.append(name.module);
		if (!name.part.empty()) {
			fname.push_back(partition_separator_);
			fname.append(name.part);
		}
		fname.append(ext_);
		return fname;
	}

	std::optional<artifact> binary_interface::from_module(
	    include_locator& locator,
	    std::filesystem::path const& source_path,
	    mod_name const& ref) {
		if (!ref.module.empty() &&
		    (ref.module.front() == u8'<' || ref.module.front() == u8'"')) {
			return header_module(locator, source_path, ref);
		}
		return mod_ref{ref, as_interface(ref)};
	}

	void binary_interface::add_targets(std::vector<target>& targets,
	                                   rule_types& rules_needed) const {
		for (auto const& [bmi_file, sources] : header_modules_) {
			rules_needed.set(rule_type::EMIT_INCLUDE);
			target bmi{rule_type::EMIT_INCLUDE,
			           file_ref{0, bmi_file, file_ref::header_module,
			                    std::get<1>(sources)}};
			bmi.inputs.expl.push_back(file_ref{0, std::get<0>(sources),
			                                   file_ref::include,
			                                   std::get<2>(sources)});
			targets.push_back(std::move(bmi));
			targets.push_back({
			    {},
			    file_ref{0, std::get<0>(sources), file_ref::include,
			             std::get<2>(sources)},
			});
		}
	}

	std::optional<artifact> binary_interface::header_module(
	    include_locator& locator,
	    std::filesystem::path const& source_path,
	    mod_name const& ref) {
		auto const path = locator.find_include(source_path, ref.module);
		if (path.empty()) return std::nullopt;

		auto const bmi_rel = path.relative_path().generic_u8string() + ext_;
		std::u8string bmi{};
		bmi.reserve(dirname_.size() + bmi_rel.size());
		bmi.append(dirname_);
		bmi.append(bmi_rel);
		auto const bmi_node_name =
		    std::filesystem::path{bmi}.filename().generic_u8string();
		header_modules_[bmi] = {
		    path.generic_u8string(),
		    bmi_node_name,
		    ref.module,
		};
		return file_ref{0, std::move(bmi), file_ref::header_module,
		                bmi_node_name};
	}
}  // namespace env
