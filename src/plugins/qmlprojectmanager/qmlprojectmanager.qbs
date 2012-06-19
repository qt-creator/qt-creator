import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "QmlProjectManager"

    Depends { name: "Qt"; submodules: ["widgets", "quick1"] }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "QmlJSEditor" }
    Depends { name: "QmlJS" }
    Depends { name: "Debugger" }
    Depends { name: "QtSupport" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "fileformat/qmlprojectitem.h",
        "fileformat/filefilteritems.h",
        "fileformat/qmlprojectfileformat.h",
        "qmlproject.h",
        "qmlprojectplugin.h",
        "qmlprojectmanager.h",
        "qmlprojectconstants.h",
        "qmlprojectnodes.h",
        "qmlprojectfile.h",
        "qmlprojectruncontrol.h",
        "qmlprojectrunconfiguration.h",
        "qmlprojectrunconfigurationfactory.h",
        "qmlprojectapplicationwizard.h",
        "qmlprojectmanager_global.h",
        "qmlprojectmanagerconstants.h",
        "qmlprojecttarget.h",
        "qmlprojectrunconfigurationwidget.h",
        "fileformat/qmlprojectitem.cpp",
        "fileformat/filefilteritems.cpp",
        "fileformat/qmlprojectfileformat.cpp",
        "qmlproject.cpp",
        "qmlprojectplugin.cpp",
        "qmlprojectmanager.cpp",
        "qmlprojectnodes.cpp",
        "qmlprojectfile.cpp",
        "qmlprojectruncontrol.cpp",
        "qmlprojectrunconfiguration.cpp",
        "qmlprojectrunconfigurationfactory.cpp",
        "qmlprojectapplicationwizard.cpp",
        "qmlprojecttarget.cpp",
        "qmlprojectrunconfigurationwidget.cpp",
        "qmlproject.qrc",
        "QmlProject.mimetypes.xml"
    ]
}

