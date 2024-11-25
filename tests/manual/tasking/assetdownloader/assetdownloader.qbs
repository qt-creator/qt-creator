QtcManualTest {
    name: "Tasking assetdownloader"
    type: ["application"]

    Depends { name: "Qt"; submodules: ["concurrent", "network", "widgets", "core-private", "gui-private"] }
    Depends { name: "Tasking" }

    files: [
        "assetdownloader.cpp",
        "assetdownloader.h",
        "main.cpp",
    ]
}
