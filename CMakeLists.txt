cmake_minimum_required (VERSION 3.10)
project (c++modules)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${EXTRA_CFLAGS} /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${EXTRA_CFLAGS} /MT")

set(ADDITIONAL_WALL_FLAGS
    /std:c++latest
    /permissive-
    /Zc:__cplusplus
    /W4
    /w14242
    /w14254
    /w14263
    /w14265
    /w14287
    /we4289
    /w14296
    /w14311
    /w14545
    /w14546
    /w14547
    /w14549
    /w14555
    /w14619
    /w14640
    /w14826
    /w14905
    /w14906
    /w14928
    /w14946
    )
else()
set(ADDITIONAL_WALL_FLAGS
    -std=c++17
    -Wall -Wextra
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  list(APPEND ADDITIONAL_WALL_FLAGS -fcolor-diagnostics) # -Wlifetime
else()
  list(APPEND ADDITIONAL_WALL_FLAGS
    -fconcepts
    -fdiagnostics-color
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
    )
endif()
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(external)
add_subdirectory(libs)
add_subdirectory(extras)

# add_executable(hilite-test test.cc)
# target_compile_options(hilite-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
# target_link_libraries(hilite-test PUBLIC cell hilite hilite-cxx fs)

add_executable(c++modules
  src/compilers/cl.cc
  src/compilers/cl.hh
  src/compilers/clang.cc
  src/compilers/clang.hh
  src/compilers/gcc.cc
  src/compilers/gcc.hh
  src/compiler.cc
  src/compiler.hh
  src/generators/dot.cc
  src/generators/dot.hh
  src/generators/ninja.cc
  src/generators/ninja.hh
  src/generator.cc
  src/generator.hh
  src/logger.cc
  src/logger.hh
  src/scanner.cc
  src/scanner.hh
  src/types.cc
  src/types.hh
  src/main.cc)
target_compile_options(c++modules PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_libraries(c++modules PUBLIC cell hilite hilite-cxx fs json tiny-process-library)