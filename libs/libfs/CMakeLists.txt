set(LIBS)
if (NOT MSVC)
  list(APPEND LIBS stdc++fs)
endif()

add_library(fs STATIC src/file.cc include/fs/file.hh)
target_compile_options(fs PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_include_directories(fs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fs PUBLIC ${LIBS})
set_target_properties(fs PROPERTIES FOLDER libs)
