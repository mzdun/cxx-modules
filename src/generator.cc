#include "generator.hh"

using namespace std::literals;

generator::~generator() = default;

templated_string rule::default_message(rule_type type) {
	switch (type) {
		case rule_type::MKDIR:
			return {"Create DIR "s, var::OUTPUT};
		case rule_type::COMPILE:
			return {"Building CXX object "s, var::OUTPUT};
		case rule_type::EMIT_BMI:
			return {"Building CXX module interface "s, var::OUTPUT};
		case rule_type::EMIT_INCLUDE:
			return {"Building CXX header-module interface "s, var::OUTPUT};
		case rule_type::ARCHIVE:
			return {"Linking CXX static library "s, var::OUTPUT};
		case rule_type::LINK_SO:
			return {"Linking CXX shared library "s, var::OUTPUT};
		case rule_type::LINK_MOD:
			return {"Linking CXX module library "s, var::OUTPUT};
		case rule_type::LINK_EXECUTABLE:
			return {"Linking CXX executable "s, var::OUTPUT};
	}
	return {};
}