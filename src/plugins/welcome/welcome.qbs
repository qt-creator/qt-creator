import qbs 1.0

QtcPlugin {
    name: "Welcome"

    Depends { name: "Qt"; submodules: ["widgets", "network" ] }
    Depends { name: "Utils" }

    Depends { name: "Core" }

    files: [
        "introductionwidget.cpp",
        "introductionwidget.h",
        "welcomeplugin.cpp",
        "welcometr.h",
    ]

    Group {
        name: "images"
        prefix: "images/"
        files: [
            "border.png",
            "mode_welcome.png",
            "mode_welcome_mask.png",
            "mode_welcome_mask@2x.png",
            "mode_welcome@2x.png",
            "link.png",
            "link@2x.png",
        ]
        fileTags: "qt.core.resource_data"
    }
}
