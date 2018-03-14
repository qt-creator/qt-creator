import qbs 1.0

DynamicLibrary {
    name: "Simple Library"

    Depends { name: 'cpp' }

    files: [
        "simple-library.cpp",
        "simple-library.h",
    ]
}

