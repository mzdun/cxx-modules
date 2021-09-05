#pragma once

#include <string>
#include <string_view>

inline std::string_view as_sv(std::u8string_view view) {
	return {reinterpret_cast<char const*>(view.data()), view.size()};
}

inline std::u8string_view as_u8sv(std::string_view view) {
	return {reinterpret_cast<char8_t const*>(view.data()), view.size()};
}

inline std::string as_str(std::u8string_view view) {
	return {reinterpret_cast<char const*>(view.data()), view.size()};
}

inline std::string as_str(std::string_view view) {
	return {view.data(), view.size()};
}

inline std::u8string as_u8str(std::u8string_view view) {
	return {view.data(), view.size()};
}

inline std::vector<std::string> split_s(char sep, std::string_view data) {
	std::vector<std::string> result{};

	auto pos = data.find(sep);
	decltype(pos) prev = 0;

	while (pos != std::string_view::npos) {
		auto const view = data.substr(prev, pos - prev);
		prev = pos + 1;
		pos = data.find(sep, prev);
		result.push_back(as_str(view));
	}

	result.push_back(as_str(data.substr(prev)));

	return result;
}

inline std::string_view lstrip_sv(std::string_view data) {
	auto new_stop = data.size();
	decltype(new_stop) new_start = 0;

	while (new_start < new_stop &&
	       std::isspace(static_cast<unsigned char>(data[new_start])))
		++new_start;

	return data.substr(new_start);
}

inline std::string_view rstrip_sv(std::string_view data) {
	auto new_stop = data.size();

	while (new_stop > 0 &&
	       std::isspace(static_cast<unsigned char>(data[new_stop - 1])))
		--new_stop;

	return data.substr(0, new_stop);
}

inline std::string_view strip_sv(std::string_view data) {
	return lstrip_sv(rstrip_sv(data));
}

inline std::string strip_s(std::string_view data) {
	auto const result = strip_sv(data);
	return {result.data(), result.size()};
}

inline std::string lstrip_s(std::string_view data) {
	auto const result = lstrip_sv(data);
	return {result.data(), result.size()};
}

inline std::string rstrip_s(std::string_view data) {
	auto const result = rstrip_sv(data);
	return {result.data(), result.size()};
}

template <typename... String>
inline std::u8string u8concat(String const&... parts) {
	auto const length = (parts.length() + ...);
	std::u8string result{};
	result.reserve(length);
	(result.append(parts), ...);
	return result;
}
