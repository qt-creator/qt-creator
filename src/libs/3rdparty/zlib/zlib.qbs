StaticLibrary {
    name: "qtcZLib"
    builtByDefault: false
    Depends { name: "cpp" }
    Group {
        name: "sources"
        prefix: "src/"
        files: [
            "adler32.c",
            "compress.c",
            "crc32.c",
            "crc32.h",
            "deflate.c",
            "deflate.h",
            "gzclose.c",
            "gzguts.h",
            "gzlib.c",
            "gzread.c",
            "gzwrite.c",
            "infback.c",
            "inffast.c",
            "inffast.h",
            "inffixed.h",
            "inflate.c",
            "inflate.h",
            "inftrees.c",
            "inftrees.h",
            "trees.c",
            "trees.h",
            "uncompr.c",
            "zconf.h",
            "zlib.h",
            "zutil.c",
            "zutil.h",
        ]
    }
    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cFlags: "/wd4996"
    }

    cpp.includePaths: "src"
    Export {
        Depends { name: "cpp" }
        cpp.includePaths: exportingProduct.sourceDirectory + "/src"
    }
}
