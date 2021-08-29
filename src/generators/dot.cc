#include "dot.hh"
#include <fstream>
#include <map>
#include <set>

#include "../utils.hh"

using namespace std::literals;

namespace {
	std::string_view name2shape(rule_name const& name) {
		return std::visit(
		    [](auto const& name) -> std::string_view {
			    if constexpr (std::is_same_v<decltype(name),
			                                 rule_type const&>) {
				    switch (name) {
					    case rule_type::MKDIR:
						    return {};
					    case rule_type::COMPILE:
						    return {};
					    case rule_type::EMIT_BMI:
						    return "hexagon"sv;
					    case rule_type::ARCHIVE:
						    return "septagon"sv;
					    case rule_type::LINK_SO:
						    return "pentagon"sv;
					    case rule_type::LINK_MOD:
						    return "octagon"sv;
					    case rule_type::LINK_EXECUTABLE:
						    return "rect"sv;
				    }
			    } else if constexpr (std::is_same_v<decltype(name),
			                                        std::monostate const&>) {
				    return "house"sv;
			    }
			    return {};
		    },
		    name);
	}

	bool ignorable(rule_name const& name) {
		return std::visit(
		    [](auto const& name) -> bool {
			    if constexpr (std::is_same_v<decltype(name),
			                                 rule_type const&>) {
				    switch (name) {
					    case rule_type::MKDIR:
						    return true;
					    case rule_type::COMPILE:
					    case rule_type::EMIT_BMI:
					    case rule_type::ARCHIVE:
					    case rule_type::LINK_SO:
					    case rule_type::LINK_MOD:
					    case rule_type::LINK_EXECUTABLE:
						    break;
				    }
			    }
			    return false;
		    },
		    name);
	}

	std::u8string printable(artifact const& file) {
		return std::visit(
		    [](auto const& filename) -> std::u8string { return filename.path; },
		    file);
	}
}  // namespace

void dot::generate(std::filesystem::path const&,
                   std::filesystem::path const& builddir) {
	std::ofstream dotfile{builddir / u8"dependencies.dot"sv};

	dotfile << "digraph {\n"
	           "    node [fontname=\"Atkinson Hyperlegible\"]\n"
	           "    edge [fontname=\"Atkinson Hyperlegible\"]\n"
	           "\n";

	std::set<artifact> ignored;
	std::map<artifact, std::string> nodeIds;
	size_t counter = 0;

	for (auto const& target : targets_) {
		if (ignorable(target.rule)) {
			ignored.insert(target.main_output);
			ignored.insert(target.outputs.expl.begin(),
			               target.outputs.expl.end());
			ignored.insert(target.outputs.impl.begin(),
			               target.outputs.impl.end());
			ignored.insert(target.outputs.order.begin(),
			               target.outputs.order.end());
			continue;
		}

		++counter;
		auto const shape = name2shape(target.rule);
		auto nodeId = "node" + std::to_string(counter);

		dotfile << "    " << nodeId << " [label=\""
		        << as_sv(printable(target.main_output));
		if (!shape.empty()) dotfile << "\" shape=\"" << shape;
		dotfile << "\"]\n";
		nodeIds[target.main_output] = std::move(nodeId);
	}

	for (auto const& target : targets_) {
		if (ignorable(target.rule)) continue;
		auto srcNode = nodeIds.find(target.main_output);
		if (srcNode == nodeIds.end()) continue;

		bool first = true;
		for (auto const& in : target.inputs.expl) {
			if (ignored.count(in) != 0) continue;
			auto dstNode = nodeIds.find(in);
			if (dstNode == nodeIds.end()) continue;
			if (first) {
				first = false;
				dotfile << "    " << srcNode->second << " -> {";
			}
			dotfile << ' ' << dstNode->second;
		}

		for (auto const& in : target.inputs.impl) {
			if (ignored.count(in) != 0) continue;
			auto dstNode = nodeIds.find(in);
			if (dstNode == nodeIds.end()) continue;
			if (first) {
				first = false;
				dotfile << "    " << srcNode->second << " -> {";
			}
			dotfile << ' ' << dstNode->second;
		}

		if (!first) {
			dotfile << " }";
			if (!target.edge.empty()) {
				dotfile << " [label=\"" << as_sv(target.edge) << "\"]";
			}
			dotfile << '\n';
		}

		first = true;
		for (auto const& in : target.inputs.order) {
			if (ignored.count(in) != 0) continue;

			std::string_view name{};

			auto dstNode = nodeIds.find(in);
			if (dstNode == nodeIds.end()) {
				for (auto const& other : targets_) {
					bool found = other.main_output == in;
					if (!found) {
						for (auto const& out : other.outputs.impl) {
							if (out == in) {
								found = true;
								break;
							}
						}
					}
					if (!found) {
						for (auto const& out : other.outputs.order) {
							if (out == in) {
								found = true;
								break;
							}
						}
					}
					if (!found) {
						for (auto const& out : other.outputs.expl) {
							if (out == in) {
								found = true;
								break;
							}
						}
					}
					if (found) {
						dstNode = nodeIds.find(other.main_output);
						break;
					}
				}
			}

			if (dstNode != nodeIds.end()) {
				name = dstNode->second;
			}

			if (name.empty()) continue;

			if (first) {
				first = false;
				dotfile << "    " << srcNode->second << " -> {";
			}
			dotfile << ' ' << name;
		}

		if (!first) {
			dotfile << " } [style=dashed]\n";
		}
	}
	/*for (auto const& target : targets_) {
	    if (ignorable(target.rule)) continue;

	    dotfile << "build " << as_sv(target.main_output);
	    for (auto const& in : target.outputs.expl)
	        dotfile << ' ' << as_sv(in);
	    if (!target.outputs.impl.empty() || !target.outputs.order.empty())
	        dotfile << " |";
	    for (auto const& in : target.outputs.impl)
	        dotfile << ' ' << as_sv(in);
	    for (auto const& in : target.outputs.order)
	        dotfile << ' ' << as_sv(in);

	    for (auto const& in : target.inputs.expl) {
	        if (ignored.count(in) != 0) continue;
	        dotfile << ' ' << as_sv(in);
	    }
	    bool first = true;
	    for (auto const& in : target.inputs.impl) {
	        if (ignored.count(in) != 0) continue;
	        if (first) {
	            first = false;
	            dotfile << " |";
	        }
	        dotfile << ' ' << as_sv(in);
	    }
	    first = true;
	    for (auto const& in : target.inputs.order) {
	        if (ignored.count(in) != 0) continue;
	        if (first) {
	            first = false;
	            dotfile << " ||";
	        }
	        dotfile << ' ' << as_sv(in);
	    }
	    dotfile << '\n';
	}*/

	dotfile << "}\n";
}