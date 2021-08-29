set(SOURCES
  src/ascii.cc
  src/character.cc
  src/context.cc
  src/operators.cc
  src/parser.cc
  src/repeat_operators.cc
  src/string.cc
  src/special.cc
  src/tokens.cc

  include/cell/ascii.hh
  include/cell/character.hh
  include/cell/context.hh
  include/cell/operators.hh
  include/cell/parser.hh
  include/cell/repeat_operators.hh
  include/cell/string.hh
  include/cell/special.hh
  include/cell/tokens.hh
  )

add_library(cell STATIC ${SOURCES})
target_compile_options(cell PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_include_directories(cell PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(cell PROPERTIES FOLDER libs)
