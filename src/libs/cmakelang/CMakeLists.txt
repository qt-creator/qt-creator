set(CMAKELANG_PARSER_FILES
  cmakeparser.h
  cmakeparser.cpp
  cmakeparsertable_p.h
  cmakeparsertable.cpp
)

foreach(file IN LISTS CMAKELANG_PARSER_FILES)
  list(APPEND CMAKELANG_GENERATED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/${file}")
endforeach()

# qlalr writes its output into the working directory and spells the grammar the
# way it was passed into the #line directives, so it runs on a copy of the
# grammar in the build directory.  What it produces is what gets compiled; the
# copies next to the grammar are refreshed only where they differ, so a build
# leaves the source tree alone unless the grammar really changed.
add_custom_command(
  OUTPUT ${CMAKELANG_GENERATED_SOURCES}
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

set_source_files_properties(cmakelang.g PROPERTIES HEADER_FILE_ONLY ON)

add_qtc_library(CMakeLang
  DEPENDS Qt::Core
  SOURCES
    cmakelang.g
    cmakelang.h
    cmakeast.cpp cmakeast.h
    cmakeastvisitor.cpp cmakeastvisitor.h
    cmakedocument.cpp cmakedocument.h
    cmakeengine.cpp cmakeengine.h
    cmakelexer.cpp cmakelexer.h
    cmakememorypool.h
    cmakerewriter.cpp cmakerewriter.h
    cmakesignature.cpp cmakesignature.h
    ${CMAKELANG_GENERATED_SOURCES}
)
