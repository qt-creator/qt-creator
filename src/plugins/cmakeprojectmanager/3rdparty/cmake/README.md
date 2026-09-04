Files taken from the CMake repository https://gitlab.kitware.com/cmake/cmake.git

624461526f4707a2406ebbd40245a605b6bd41fa (tag: v3.26.3)

cmListFileCache.h holds the upstream data structures. cmListFileCache.cxx no
longer contains the upstream parser: it fills those structures from the AST
that src/libs/cmakelang produces.
