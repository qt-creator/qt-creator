import qbs

QtcLibrary {
    name: "Parsing"

    cpp.defines: base.concat([
        "PARSING_LIBRARY"
    ])
    cpp.includePaths: base.concat([sourceDirectory])

    files: [
        "engine.cpp",
        "engine.h",
        "memorypool.h",
        "parsing_global.h",
    ]
}
