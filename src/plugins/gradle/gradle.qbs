import qbs 1.0

QtcPlugin {
    name: "Gradle"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Qt"; submodules: ["widgets"] }

    files: [
        "gradleplugin.cpp",
        "gradleproject.cpp",
        "gradleproject.h",
    ]
}

