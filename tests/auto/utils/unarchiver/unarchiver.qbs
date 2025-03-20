import qbs

QtcAutotest {
    name: "Unarchiver autotest"
    Depends { name: "libarchive_static" }
    Depends { name: "Utils" }

    cpp.includePaths: libarchive_static.libarchiveIncludeDir
    cpp.libraryPaths: libarchive_static.libarchiveLibDir
    cpp.staticLibraries: libarchive_static.libarchiveStatic
                         ? libarchive_static.libarchiveNames : []
    cpp.dynamicLibraries: !libarchive_static.libarchiveStatic
                          ? libarchive_static.libarchiveNames : []

    files: "tst_unarchiver.cpp"
}
