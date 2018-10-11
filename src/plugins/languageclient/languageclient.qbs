import qbs 1.0

QtcPlugin {

    name: "LanguageClient"

    Depends { name: "Qt.core" }

    Depends { name: "Utils" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "LanguageServerProtocol" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    files: [
        "baseclient.cpp",
        "baseclient.h",
        "dynamiccapabilities.cpp",
        "dynamiccapabilities.h",
        "languageclient.qrc",
        "languageclient_global.h",
        "languageclientcodeassist.cpp",
        "languageclientcodeassist.h",
        "languageclientmanager.cpp",
        "languageclientmanager.h",
        "languageclientplugin.cpp",
        "languageclientplugin.h",
        "languageclientsettings.cpp",
        "languageclientsettings.h",
    ]
}
