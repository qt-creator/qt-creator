QtcLibrary {
    name: "Nanotrace"

    cpp.defines: base.concat("NANOTRACE_LIBRARY", "NANOTRACE_ENABLED")
    cpp.visibility: "default"

    files: [
        "nanotrace.cpp",
        "nanotrace.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.defines: "NANOTRACE_ENABLED"
    }
}
