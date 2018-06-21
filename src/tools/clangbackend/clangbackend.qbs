import qbs 1.0

QtcTool {
    name: "clangbackend"

    Depends { name: "ClangSupport" }
    Depends { name: "libclang"; required: false }

    Group {
        prefix: "source/"
        files: [
            "*.h",
            "*.cpp"
        ]
    }

    Group {
        prefix: "../qtcreatorcrashhandler/"
        files: [
            "crashhandlersetup.cpp",
            "crashhandlersetup.h",
        ]
    }

    files: [ "clangbackendmain.cpp" ]

    condition: libclang.present

    cpp.includePaths: base.concat(["source", libclang.llvmIncludeDir])
    cpp.libraryPaths: base.concat(libclang.llvmLibDir)
    cpp.dynamicLibraries: base.concat(libclang.llvmLibs)
    cpp.rpaths: base.concat(libclang.llvmLibDir)

    Properties {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("macos")
        cpp.linkerFlags: base.concat(["-z", "origin"])
    }
}
