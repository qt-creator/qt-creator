QtcPlugin {
    name: "ZenMode"

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: [ "widgets" ] }

    files: [
        "zenmodeplugin.cpp",
        "zenmodepluginconstants.h",
        "zenmodeplugintr.h",
        "zenmodesettings.cpp",
        "zenmodesettings.h",
    ]

    Group {
        name: "images"
        fileTags: "qt.core.resource_data"
        files: [
            "images/settingscategory_zenmode.png",
            "images/settingscategory_zenmode@2x.png",
            "images/zenmode.png",
            "images/zenmode@2x.png",
            "images/distractionfree.png",
            "images/distractionfree@2x.png",
        ]
    }
}
