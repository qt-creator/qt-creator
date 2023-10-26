QtcManualTest {
    name: "Tasking dataexchange"
    type: ["application"]

    Depends { name: "Qt"; submodules: ["concurrent", "network", "widgets"] }
    Depends { name: "Tasking" }

    files: [
        "main.cpp",
        "recipe.cpp",
        "recipe.h",
        "viewer.cpp",
        "viewer.h",
    ]
}
