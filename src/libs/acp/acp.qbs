QtcLibrary {
    name: "AcpLib"
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["core"] }

    cpp.defines: base.concat("ACPLIB_LIBRARY")

    files: [
        "acp.cpp",
        "acp.h",
        "acp_global.h",
        "acpregistry.cpp",
        "acpregistry.h",
        "acpv2.cpp",
        "acpv2.h",
    ]
}
