QtcLibrary {
    name: "LanguageServerProtocol"
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["core"] }

    cpp.defines: base.concat("LANGUAGESERVERPROTOCOL_LIBRARY")

    files: [
        "languageserverprotocol_global.h",
        "languageserverprotocoltr.h",
        "lspbasemessage.cpp",
        "lspbasemessage.h",
        "lspjsonrpc.cpp",
        "lspjsonrpc.h",
        "lspmessages.h",
        "lsputils.cpp",
        "lsputils.h",
        "lsptypes.cpp",
        "lsptypes.h",
    ]

    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cxxFlags: "/bigobj"
    }
    Properties {
        condition: qbs.toolchain.contains("mingw")
        cpp.cxxFlags: "-Wa,-mbig-obj"
    }
}
