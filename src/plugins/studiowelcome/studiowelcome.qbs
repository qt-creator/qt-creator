import qbs.FileInfo

QtcPlugin {
    name: "StudioWelcome"

    Depends { name: "Qt"; submodules: ["qml", "quick", "quickwidgets"] }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "QmlDesigner" }
    Depends { name: "QmlProjectManager" }
    Depends { name: "app_version_header" }

    cpp.defines: 'STUDIO_QML_PATH="' + FileInfo.joinPaths(sourceDirectory, "qml") + '"'

    files: [
        "createproject.cpp",
        "createproject.h",
        "examplecheckout.h",
        "examplecheckout.cpp",
        "newprojectdialogimageprovider.h",
        "newprojectdialogimageprovider.cpp",
        "presetmodel.cpp",
        "presetmodel.h",
        "qdsnewdialog.cpp",
        "qdsnewdialog.h",
        "screensizemodel.h",
        "studiowelcome_global.h",
        "studiowelcomeplugin.h",
        "studiowelcomeplugin.cpp",
        "studiowelcome.qrc",
        "stylemodel.cpp",
        "stylemodel.h",
        "wizardfactories.cpp",
        "wizardfactories.h",
        "wizardhandler.cpp",
        "wizardhandler.h",
        "recentpresets.cpp",
        "recentpresets.h"
    ]

    Group {
        name: "studiofonts"
        prefix: "../../share/3rdparty/studiofonts/"
        files: "studiofonts.qrc"
    }

    Qt.core.resourceFileBaseName: "StudioWelcome_qml"
    Qt.core.resourceSourceBase: "."
    Group {
        name: "Qml Files"
        fileTags: "qt.core.resource_data"
        files: "qml/**"
    }
}
