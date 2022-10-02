# C++ modules

Naïve C++20 module scanner

|Compiler/Build System|Comments|
|---------------------|--------|
| gcc 11 / Ninja | Compiles, segfaults on `std::string` copy constructor |
| clang 14 / Ninja | Does not compile yet |
| Visual Studio 2022 17 / MSBuild | Compiles |
