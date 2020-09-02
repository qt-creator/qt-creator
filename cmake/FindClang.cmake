find_package(Clang CONFIG)

# silence a lot of warnings from building against llvm
if(MSVC AND TARGET libclang)
    target_compile_options(libclang INTERFACE /wd4100 /wd4141 /wd4146 /wd4244 /wd4267 /wd4291)
endif()

option(CLANGTOOLING_LINK_CLANG_DYLIB "Force linking of Clang tooling against clang-cpp" NO)

if (TARGET clangTooling AND NOT CLANGTOOLING_LINK_CLANG_DYLIB)
  set(CLANG_TOOLING_LIBS libclang clangTooling clangQuery clangIndex)
elseif (TARGET clang-cpp)
  set(CLANG_TOOLING_LIBS libclang clang-cpp)
endif()
