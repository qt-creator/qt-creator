set(RSTLANG_PARSER_FILES
  rstparser.h
  rstparser.cpp
  rstparsertable_p.h
  rstparsertable.cpp
)

# The parser qlalr writes is checked in, the way the one of the GLSL library
# is: a build then asks for no host tool beyond the ones Qt itself needs.  With
# the option below qlalr runs and writes the parser into the source tree again,
# so that what is compiled is what is committed.
option(QTC_RSTLANG_AUTOGENERATE_PARSER
  "Regenerate the reStructuredText parser from rstlang.g with qlalr" OFF)

if(QTC_RSTLANG_AUTOGENERATE_PARSER)
  foreach(file IN LISTS RSTLANG_PARSER_FILES)
    list(APPEND rstlang_parser_outputs "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
  endforeach()

  # qlalr writes its output into the working directory and spells the grammar
  # the way it was passed into the #line directives, so it runs on a copy of
  # the grammar in the build directory.
  add_custom_command(
    OUTPUT ${rstlang_parser_outputs}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/rstlang.g" rstlang.g
    COMMAND Qt6::qlalr --qt --no-debug ./rstlang.g
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${RSTLANG_PARSER_FILES} "${CMAKE_CURRENT_SOURCE_DIR}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rstlang.g" Qt6::qlalr
    COMMENT "Generating the reStructuredText parser"
    VERBATIM
  )
endif()

set_source_files_properties(rstlang.g PROPERTIES HEADER_FILE_ONLY ON)

add_qtc_library(RstLang
  PUBLIC_DEPENDS Parsing
  DEPENDS Qt::Core
  SOURCES
    rstlang.g
    rstlang.h
    rstast.cpp rstast.h
    rstastvisitor.cpp rstastvisitor.h
    rstdocument.cpp rstdocument.h
    rstlexer.cpp rstlexer.h
    rstmarkdown.cpp rstmarkdown.h
    ${RSTLANG_PARSER_FILES}
)
