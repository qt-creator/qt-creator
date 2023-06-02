import qbs 1.0

QtcPlugin {
    name: "QmlProjectManager"

    Depends { name: "Qt"; submodules: ["widgets", "network", "quickwidgets"] }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmlDesignerBase" }
    Depends { name: "QtSupport" }
    Depends { name: "TextEditor" }

    Group {
        name: "General"
        files: [
            "projectfilecontenttools.cpp", "projectfilecontenttools.h",
            "qdslandingpagetheme.cpp", "qdslandingpagetheme.h",
            "qdslandingpage.cpp", "qdslandingpage.h",
            "qmlmainfileaspect.cpp", "qmlmainfileaspect.h",
            "qmlmultilanguageaspect.cpp", "qmlmultilanguageaspect.h",
            "qmlproject.cpp", "qmlproject.h",
            "qmlproject.qrc",
            "qmlprojectconstants.h",
            "qmlprojectmanager_global.h", "qmlprojectmanagertr.h",
            "qmlprojectmanagerconstants.h",
            "qmlprojectplugin.cpp", "qmlprojectplugin.h",
            "qmlprojectrunconfiguration.cpp", "qmlprojectrunconfiguration.h",
            project.ide_source_tree + "/src/share/3rdparty/studiofonts/studiofonts.qrc"
        ]
    }

    Group {
        name: "Build System"
        prefix: "buildsystem/"
        files: [
            "qmlbuildsystem.cpp", "qmlbuildsystem.h",
            "projectitem/filefilteritems.cpp", "projectitem/filefilteritems.h",
            "projectitem/qmlprojectitem.cpp", "projectitem/qmlprojectitem.h",
            "projectitem/converters.cpp", "projectitem/converters.h",
            "projectnode/qmlprojectnodes.cpp", "projectnode/qmlprojectnodes.h"
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

    Group {
        name: "QML Project File Generator"
        prefix: "qmlprojectgen/"
        files: [
            "qmlprojectgenerator.cpp", "qmlprojectgenerator.h",
            "templates.qrc"
        ]
    }
}
