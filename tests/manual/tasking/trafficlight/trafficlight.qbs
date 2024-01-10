QtcManualTest {
    name: "Tasking trafficlight"
    type: ["application"]

    Depends { name: "Qt"; submodules: ["concurrent", "network", "widgets"] }
    Depends { name: "Tasking" }

    files: [
        "glueinterface.h",
        "main.cpp",
        "recipe.cpp",
        "recipe.h",
        "trafficlight.cpp",
        "trafficlight.h",
        "trafficlight.qrc",
    ]
}
