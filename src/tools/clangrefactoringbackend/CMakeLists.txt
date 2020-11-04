add_subdirectory(source)

add_qtc_executable(clangrefactoringbackend
  CONDITION TARGET ClangRefactoring
  DEPENDS
    clangrefactoringbackend_lib Sqlite ClangSupport
  SOURCES
    clangrefactoringbackendmain.cpp
)
