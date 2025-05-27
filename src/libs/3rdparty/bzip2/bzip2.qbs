StaticLibrary {
    name: "qtcBZip2"
    builtByDefault: false
    Depends { name: "cpp" }
    files: [
        "blocksort.c",
        "bzlib.c",
        "bzlib.h",
        "compress.c",
        "crctable.c",
        "decompress.c",
        "huffman.c",
        "randtable.c",
    ]
    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cFlags: ["/wd4244", "/wd4267", "/wd4996"]
    }
    Properties {
        condition: qbs.toolchain.contains("gcc")
        cpp.cFlags: "-Wno-unused-parameter"
    }
    Properties {
        condition: qbs.toolchain.contains("gcc") && !qbs.toolchain.contains("clang")
        cpp.cFlags: "-Wimplicit-fallthrough=0"
    }
    cpp.includePaths: "."
    Export {
        Depends { name: "cpp" }
        cpp.includePaths: exportingProduct.sourceDirectory
    }
}
