add_subdirectory(source)

add_qtc_executable(clangpchmanagerbackend
  BUILD_DEFAULT OFF
  DEPENDS
    clangrefactoringbackend_lib clangpchmanagerbackend_lib
    Sqlite ClangSupport
    ${CLANG_TOOLING_LIBS}
  SOURCES
    clangpchmanagerbackendmain.cpp
)
