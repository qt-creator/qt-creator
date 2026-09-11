set(CMAKELANG_PARSER_FILES
  cmakeparser.h
  cmakeparser.cpp
  cmakeparsertable_p.h
  cmakeparsertable.cpp
)

# The parser qlalr writes is checked in, the way the one of the GLSL library
# is: a build then asks for no host tool beyond the ones Qt itself needs.  With
# the option below qlalr runs and writes the parser into the source tree again,
# so that what is compiled is what is committed.
option(QTC_CMAKELANG_AUTOGENERATE_PARSER
  "Regenerate the CMake language parser from cmakelang.g with qlalr" OFF)

if(QTC_CMAKELANG_AUTOGENERATE_PARSER)
  foreach(file IN LISTS CMAKELANG_PARSER_FILES)
    list(APPEND cmakelang_parser_outputs "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
  endforeach()

  # qlalr writes its output into the working directory and spells the grammar
  # the way it was passed into the #line directives, so it runs on a copy of
  # the grammar in the build directory.
  add_custom_command(
    OUTPUT ${cmakelang_parser_outputs}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/cmakelang.g" cmakelang.g
    COMMAND Qt6::qlalr --qt --no-debug ./cmakelang.g
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKELANG_PARSER_FILES} "${CMAKE_CURRENT_SOURCE_DIR}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmakelang.g" Qt6::qlalr
    COMMENT "Generating the CMake language parser"
    VERBATIM
  )
endif()

set_source_files_properties(cmakelang.g PROPERTIES HEADER_FILE_ONLY ON)

add_qtc_library(CMakeLang
  PUBLIC_DEPENDS Parsing
  DEPENDS Qt::Core
  SOURCES
    cmakelang.g
    cmakelang.h
    cmakeast.cpp cmakeast.h
    cmakeastvisitor.cpp cmakeastvisitor.h
    cmakedocument.cpp cmakedocument.h
    cmakeedit.h
    cmakeformatter.cpp cmakeformatter.h
    cmakeindentation.cpp cmakeindentation.h
    cmakelexer.cpp cmakelexer.h
    cmakerewriter.cpp cmakerewriter.h
    cmakesignature.cpp cmakesignature.h
    cmakestyle.cpp cmakestyle.h
    ${CMAKELANG_PARSER_FILES}
)
