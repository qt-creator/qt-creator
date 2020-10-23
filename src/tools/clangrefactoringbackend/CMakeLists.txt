add_subdirectory(source)

add_qtc_executable(clangrefactoringbackend
  BUILD_DEFAULT OFF
  DEPENDS
    clangrefactoringbackend_lib Sqlite ClangSupport
  SOURCES
    clangrefactoringbackendmain.cpp
)
