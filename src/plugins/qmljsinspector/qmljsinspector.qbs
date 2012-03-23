import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "QmlJSInspector"

    Depends { name: "qt"; submodules: ['gui'] }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmlProjectManager" }
    Depends { name: "TextEditor" }
    Depends { name: "Debugger" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSEditor" }
    Depends { name: "symbianutils" }
    Depends { name: "LanguageUtils" }
    Depends { name: "QmlEditorWidgets" }
    Depends { name: "QmlJSDebugClient" }


    Depends { name: "cpp" }
    cpp.includePaths: [
        ".",
        "../../libs/qmleditorwidgets",
        "../../../share/qtcreator/qml/qmljsdebugger/protocol",
        "../../shared/symbianutils",
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "qmljsprivateapi.h",
        "qmljsinspector_global.h",
        "qmljsinspectorconstants.h",
        "qmljsinspectorplugin.h",
        "qmljsclientproxy.h",
        "qmljsinspector.h",
        "qmljsinspectortoolbar.h",
        "qmljslivetextpreview.h",
        "qmljsinspectorclient.h",
        "qmljscontextcrumblepath.h",
        "qmljsinspectorsettings.h",
        "qmljspropertyinspector.h",
        "../../../share/qtcreator/qml/qmljsdebugger/protocol/inspectorprotocol.h",
        "qmljsinspectorplugin.cpp",
        "qmljsclientproxy.cpp",
        "qmljsinspector.cpp",
        "qmljsinspectortoolbar.cpp",
        "qmljslivetextpreview.cpp",
        "qmljsinspectorclient.cpp",
        "qmljscontextcrumblepath.cpp",
        "qmljsinspectorsettings.cpp",
        "qmljspropertyinspector.cpp",
        "qmljsinspector.qrc",
    ]
}

