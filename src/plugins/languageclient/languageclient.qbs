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
        "client.cpp",
        "client.h",
        "dynamiccapabilities.cpp",
        "dynamiccapabilities.h",
        "languageclient.qrc",
        "languageclient_global.h",
        "languageclientinterface.cpp",
        "languageclientinterface.h",
        "languageclientcompletionassist.cpp",
        "languageclientcompletionassist.h",
        "languageclientmanager.cpp",
        "languageclientmanager.h",
        "languageclientoutline.cpp",
        "languageclientoutline.h",
        "languageclientplugin.cpp",
        "languageclientplugin.h",
        "languageclientquickfix.cpp",
        "languageclientquickfix.h",
        "languageclientsettings.cpp",
        "languageclientsettings.h",
        "languageclientutils.cpp",
        "languageclientutils.h",
    ]
}
