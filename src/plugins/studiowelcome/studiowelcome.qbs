import qbs.FileInfo

QtcPlugin {
    name: "StudioWelcome"

    Depends { name: "Qt"; submodules: ["qml", "quick", "quickwidgets"] }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "app_version_header" }

    cpp.defines: 'STUDIO_QML_PATH="' + FileInfo.joinPaths(sourceDirectory, "qml") + '"'

    files: [
        "studiowelcome_global.h",
        "studiowelcomeplugin.h",
        "studiowelcomeplugin.cpp",
        "studiowelcome.qrc",
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
