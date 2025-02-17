QtcLibrary {
    name: "Nanotrace"

    Depends { name: "Utils" }
    Depends { name: "Qt.gui" }

    cpp.defines: base.concat("NANOTRACE_LIBRARY", "NANOTRACE_ENABLED")

    files: [
        "nanotrace.cpp",
        "nanotrace.h",
        "nanotraceglobals.h",
        "nanotracehr.cpp",
        "nanotracehr.h",
        "staticstring.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.defines: "NANOTRACE_ENABLED"
    }
}
