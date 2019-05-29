add_subdirectory(source)

add_qtc_executable(clangrefactoringbackend
  DEPENDS
    clangrefactoringbackend_lib Sqlite ClangSupport
    libclang clangTooling clangIndex clangQuery clangToolingRefactor
  SOURCES
    clangrefactoringbackendmain.cpp
)
