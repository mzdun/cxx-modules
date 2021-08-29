#pragma once

#include <cstdio>
#include <filesystem>
#include <memory>
#include <vector>

namespace fs {
	using namespace std::filesystem;

	struct fcloser {
		void operator()(FILE* f) { std::fclose(f); }
	};
	class file : private std::unique_ptr<FILE, fcloser> {
		using parent_t = std::unique_ptr<FILE, fcloser>;
		static FILE* fopen(path fname, char const* mode) noexcept;

	public:
		file();
		~file();
		file(const file&) = delete;
		file& operator=(const file&) = delete;
		file(file&&);
		file& operator=(file&&);

		explicit file(const path& fname) noexcept : file(fname, "r") {}
		file(const path& fname, const char* mode) noexcept
		    : parent_t(fopen(fname, mode)) {}

		using parent_t::operator bool;

		void close() noexcept { reset(); }
		void open(const path& fname, char const* mode = "r") noexcept {
			reset(fopen(fname, mode));
		}

		std::vector<char> read() const noexcept;
		size_t load(void* buffer, size_t length) const noexcept;
		size_t store(const void* buffer, size_t length) const noexcept;
		template <size_t Length>
		size_t print(const char (&buffer)[Length]) const noexcept {
			return store(buffer, Length - 1);
		}
		size_t print(std::string_view view) const noexcept {
			return store(view.data(), view.size());
		}
		size_t putc(char c) const noexcept {
			auto const result = fputc(c, get());
			return result != EOF;
		}
		bool skip(size_t length) const noexcept;
		bool feof() const noexcept { return std::feof(get()); }
	};

	file fopen(const path& file, char const* mode = "r") noexcept;
}  // namespace fs

#ifdef _WIN32
extern std::string oem_cp(std::string_view);
#else
#define oem_cp(x) (x)
#endif
