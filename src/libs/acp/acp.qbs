QtcLibrary {
    name: "AcpLib"
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["core"] }

    cpp.defines: base.concat("ACPLIB_LIBRARY")

    files: [
        "acp.cpp",
        "acp.h",
        "acp_global.h",
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
