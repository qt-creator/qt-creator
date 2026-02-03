QtcPlugin {
    name: "ZenModePlugin"

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: [ "widgets" ] }

    files: [
        "zenmodeplugin.cpp",
        "zenmodeplugin.h",
        "zenmodepluginconstants.h",
        "zenmodeplugintr.h",
        "zenmode.qrc",
        "zenmodesettings.cpp",
        "zenmodesettings.h",
    ]
}
