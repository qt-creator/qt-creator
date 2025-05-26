import qbs

QtcAutotest {
    name: "Unarchiver autotest"
    Depends { name: "libarchive_static" }
    Depends { name: "Utils" }

    cpp.includePaths: libarchive_static.libarchiveIncludeDir
    cpp.libraryPaths: libarchive_static.libarchiveLibDir
    cpp.staticLibraries: libarchive_static.libarchiveStatic
                         ? libarchive_static.libarchiveNames : []
    cpp.dynamicLibraries: {
        var libs = [];
        if (!libarchive_static.libarchiveStatic)
            libs = libs.concat(libarchive_static.libarchiveNames);
        if (qbs.toolchain.contains("mingw"))
            libs.push("bcrypt");
        return libs;
    }

    files: "tst_unarchiver.cpp"
}
