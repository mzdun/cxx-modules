# C++ modules

Naïve C++20 module scanner

|Compiler/Build System|Comments|
|---------------------|--------|
| gcc 11 / Ninja | Compiles, segfaults on `std::string` copy constructor |
| clang 14 / Ninja | Does not compile yet |
| cl 19 / Visual Studio | _To be done_ |
