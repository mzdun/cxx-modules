set(SOURCES
  cxx.cc
  hilite/cxx.hh
  )

add_library(hilite-cxx STATIC ${SOURCES})
target_compile_options(hilite-cxx PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_include_directories(hilite-cxx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hilite-cxx PUBLIC cell hilite)
set_target_properties(hilite-cxx PROPERTIES FOLDER libs/extras)
