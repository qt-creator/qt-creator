import qbs 1.0

QtcPlugin {
    name: "QmlProjectManager"

    Depends { name: "Qt"; submodules: ["widgets", "network", "quickwidgets"] }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "TextEditor" }

    Group {
        name: "General"
        files: [
            "projectfilecontenttools.cpp", "projectfilecontenttools.h",
            "qdslandingpage.cpp", "qdslandingpage.h",
            "qmlmainfileaspect.cpp", "qmlmainfileaspect.h",
            "qmlmultilanguageaspect.cpp", "qmlmultilanguageaspect.h",
            "qmlproject.cpp", "qmlproject.h",
            "qmlproject.qrc",
            "qmlprojectconstants.h",
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

    Group {
        name: "CMake Generator"
        prefix: "cmakegen/"
        files: [
            "generatecmakelists.cpp", "generatecmakelists.h",
            "generatecmakelistsconstants.h",
            "checkablefiletreeitem.cpp", "checkablefiletreeitem.h",
            "cmakegeneratordialogtreemodel.cpp", "cmakegeneratordialogtreemodel.h",
            "cmakegeneratordialog.cpp", "cmakegeneratordialog.h",
            "cmakeprojectconverter.cpp", "cmakeprojectconverter.h",
            "cmakeprojectconverterdialog.cpp", "cmakeprojectconverterdialog.h",
        ]
    }
}
