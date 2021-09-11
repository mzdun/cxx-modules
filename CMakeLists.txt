cmake_minimum_required (VERSION 3.10)
project (c++modules LANGUAGES CXX VERSION 0.1.0)

set(PROJECT_VERSION_STABILITY "-alpha")
set(SHAREDIR "share/c++modules-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

find_package(expat REQUIRED HINTS ${CMAKE_BINARY_DIR})
find_package(fmt REQUIRED)
find_package(args REQUIRED)

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
        /D_CRT_SECURE_NO_WARNINGS
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

configure_file(src/dirs.in.hh src/dirs.hh @ONLY)

set(SRCS
    src/base/compiler.cc
    src/base/compiler.hh
    src/base/generator.cc
    src/base/generator.hh
    src/base/logger.cc
    src/base/logger.hh
    src/base/types.cc
    src/base/types.hh
    src/base/utils.hh
    src/base/xml.hh
    src/compilers/cl.cc
    src/compilers/cl.hh
    src/cxx/scanner.cc
    src/cxx/scanner.hh
    src/env/binary_interface.cc
    src/env/binary_interface.hh
    src/env/command_list.cc
    src/env/command_list.hh
    src/env/include_locator.cc
    src/env/include_locator.hh
    src/env/path.cc
    src/env/path.hh
    src/generators/dot.cc
    src/generators/dot.hh
    src/generators/ninja.cc
    src/generators/ninja.hh
    src/main.cc
    src/xml/compiler.cc
    src/xml/compiler.hh
    src/xml/expat.hh
    src/xml/factory.cc
    src/xml/factory.hh
    src/xml/handlers-internal.hh
    src/xml/handlers.cc
    src/xml/handlers.hh
    src/xml/parser.cc
    src/xml/parser.hh
    src/xml/types.hh
    src/xml/xml.cc
)

if (MSVC)
    list(APPEND SRCS
        src/win32/vssetup.cc
        src/win32/vssetup.hh
        src/win32/Setup.Configuration.h
    )
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_executable(c++modules ${SRCS})
target_compile_options(c++modules PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_link_libraries(c++modules PUBLIC hilite-cxx fs json tiny-process-library expat::expat mbits::args)
target_include_directories(c++modules PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}/src)

add_custom_target(copy-data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/${SHAREDIR}
    COMMENT "Copy data/ to share/"
)
add_dependencies(c++modules copy-data)
