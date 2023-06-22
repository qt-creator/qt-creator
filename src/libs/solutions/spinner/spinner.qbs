QtcLibrary {
    name: "Spinner"
    Depends { name: "Qt"; submodules: ["core", "widgets"] }
    cpp.defines: base.concat("SPINNER_LIBRARY")

    files: [
        "spinner.cpp",
        "spinner.h",
        "spinner.qrc",
        "spinner_global.h",
    ]
}

