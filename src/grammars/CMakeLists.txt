# Flex is horribly broken and refuses to honor the -o command line switch.
# Ergo this hack.

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Do the Flex.
if(WIN32)
  add_custom_command(OUTPUT ntexture.lexer.c
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ntexture.lexer.flex
  ${CMAKE_CURRENT_BINARY_DIR}/ntexture.lexer.flex
  COMMAND cmd /c "${FLEX_EXECUTABLE} -PNTEXTURE_ ${CMAKE_CURRENT_BINARY_DIR}/ntexture.lexer.flex"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ntexture.lexer.flex)
else()
  add_custom_command(OUTPUT ntexture.lexer.c
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ntexture.lexer.flex
  ${CMAKE_CURRENT_BINARY_DIR}/ntexture.lexer.flex
  COMMAND ${FLEX_EXECUTABLE} -PNTEXTURE_ -o ntexture.lexer.c ${CMAKE_CURRENT_BINARY_DIR}/ntexture.lexer.flex
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ntexture.lexer.flex)
endif()

if(WIN32)
  add_custom_command(OUTPUT decorate.lexer.c
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/decorate.lexer.flex
  ${CMAKE_CURRENT_BINARY_DIR}/decorate.lexer.flex
  COMMAND cmd /c "${FLEX_EXECUTABLE} -PDECORATE_ ${CMAKE_CURRENT_BINARY_DIR}/decorate.lexer.flex"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/decorate.lexer.flex)
else()
  add_custom_command(OUTPUT decorate.lexer.c
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/decorate.lexer.flex
  ${CMAKE_CURRENT_BINARY_DIR}/decorate.lexer.flex
  COMMAND ${FLEX_EXECUTABLE} -PDECORATE_ -o decorate.lexer.c ${CMAKE_CURRENT_BINARY_DIR}/decorate.lexer.flex
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/decorate.lexer.flex)
endif()

# Do the Lemon.
if(WIN32)
  add_custom_command(OUTPUT ntexture.parser.c ntexture.parser.h
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ntexture.parser.y
  ${CMAKE_CURRENT_BINARY_DIR}/ntexture.parser.y
  COMMAND cmd /c "${LEMON_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/ntexture.parser.y"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ntexture.parser.y)
else()
  add_custom_command(OUTPUT ntexture.parser.c ntexture.parser.h
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ntexture.parser.y
  ${CMAKE_CURRENT_BINARY_DIR}/ntexture.parser.y
  COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR} ${LEMON_EXECUTABLE} ntexture.parser.y
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ntexture.parser.y)
endif()

if(WIN32)
  add_custom_command(OUTPUT decorate.parser.c decorate.parser.h
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/decorate.parser.y
  ${CMAKE_CURRENT_BINARY_DIR}/decorate.parser.y
  COMMAND cmd /c "${LEMON_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/decorate.parser.y"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/decorate.parser.y)
else()
  add_custom_command(OUTPUT decorate.parser.c decorate.parser.h
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/decorate.parser.y
  ${CMAKE_CURRENT_BINARY_DIR}/decorate.parser.y
  COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR} ${LEMON_EXECUTABLE} decorate.parser.y
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/decorate.parser.y)
endif()

set_source_files_properties(
ntexture.lexer.c
decorate.lexer.c
ntexture.parser.c
decorate.parser.c
PROPERTIES LANGUAGE CXX)

add_library(grammars
parser_driver.cpp
ntexture.lexer.c
decorate.lexer.c
ntexture.parser.c
ntexture.parser.h
decorate.parser.c
decorate.parser.h
)

target_link_libraries(grammars PUBLIC stdc++)

# Apply platform definitions needed for legacy code
target_compile_definitions(grammars PRIVATE
    FN_CDECL=
    QSORT_CALLBACK=
    NO_SHADERS
)

