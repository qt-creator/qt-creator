add_subdirectory(source)

add_qtc_executable(clangpchmanagerbackend
  DEPENDS
    clangrefactoringbackend_lib clangpchmanagerbackend_lib
    clangTooling libclang Sqlite ClangSupport clangToolingRefactor clangQuery clangIndex
  SOURCES
    clangpchmanagerbackendmain.cpp
)
