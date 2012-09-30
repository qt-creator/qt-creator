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
    Depends { name: "app_version_header" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        "..",
        "../../libs"
    ]

    files: [
        "QmlProject.mimetypes.xml",
        "qmlproject.cpp",
        "qmlproject.h",
        "qmlproject.qrc",
        "qmlprojectapplicationwizard.cpp",
        "qmlprojectapplicationwizard.h",
        "qmlprojectconstants.h",
        "qmlprojectfile.cpp",
        "qmlprojectfile.h",
        "qmlprojectmanager.cpp",
        "qmlprojectmanager.h",
        "qmlprojectmanager_global.h",
        "qmlprojectmanagerconstants.h",
        "qmlprojectnodes.cpp",
        "qmlprojectnodes.h",
        "qmlprojectplugin.cpp",
        "qmlprojectplugin.h",
        "qmlprojectrunconfiguration.cpp",
        "qmlprojectrunconfiguration.h",
        "qmlprojectrunconfigurationfactory.cpp",
        "qmlprojectrunconfigurationfactory.h",
        "qmlprojectrunconfigurationwidget.cpp",
        "qmlprojectrunconfigurationwidget.h",
        "qmlprojectruncontrol.cpp",
        "qmlprojectruncontrol.h",
        "fileformat/filefilteritems.cpp",
        "fileformat/filefilteritems.h",
        "fileformat/qmlprojectfileformat.cpp",
        "fileformat/qmlprojectfileformat.h",
        "fileformat/qmlprojectitem.cpp",
        "fileformat/qmlprojectitem.h",
    ]
}

