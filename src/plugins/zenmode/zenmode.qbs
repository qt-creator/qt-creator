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
        "zenmode.qrc",
        "zenmodesettings.cpp",
        "zenmodesettings.h",
    ]
}
