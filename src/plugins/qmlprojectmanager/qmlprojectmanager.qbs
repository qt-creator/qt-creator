import qbs 1.0

QtcPlugin {
    name: "QmlProjectManager"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "TextEditor" }

    Group {
        name: "General"
        files: [
            "qmlproject.cpp", "qmlproject.h",
            "qmlproject.qrc",
            "qmlprojectconstants.h",
            "qmlprojectenvironmentaspect.cpp", "qmlprojectenvironmentaspect.h",
            "qmlprojectmanager_global.h",
            "qmlprojectmanagerconstants.h",
            "qmlprojectnodes.cpp", "qmlprojectnodes.h",
            "qmlprojectplugin.cpp", "qmlprojectplugin.h",
            "qmlprojectrunconfiguration.cpp", "qmlprojectrunconfiguration.h"
        ]
    }

    Group {
        name: "File Format"
        prefix: "fileformat/"
        files: [
            "filefilteritems.cpp", "filefilteritems.h",
            "qmlprojectfileformat.cpp", "qmlprojectfileformat.h",
            "qmlprojectitem.cpp", "qmlprojectitem.h",
        ]
    }
}
