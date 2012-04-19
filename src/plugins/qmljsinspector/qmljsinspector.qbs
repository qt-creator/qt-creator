import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "QmlJSInspector"

    Depends { name: "qt"; submodules: ['widgets', 'quick1'] }
    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "LanguageUtils" }
    Depends { name: "TextEditor" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    Depends { name: "QmlEditorWidgets" }
    Depends { name: "QmlDebug" }


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
        "qmltoolsclient.h",
        "qmljscontextcrumblepath.h",
        "qmljsinspectorsettings.h",
        "qmljspropertyinspector.h",
        "../../../share/qtcreator/qml/qmljsdebugger/protocol/inspectorprotocol.h",
        "qmljsinspectorplugin.cpp",
        "qmljsclientproxy.cpp",
        "qmljsinspector.cpp",
        "qmljsinspectortoolbar.cpp",
        "qmljslivetextpreview.cpp",
        "qmltoolsclient.cpp",
        "qmljscontextcrumblepath.cpp",
        "qmljsinspectorsettings.cpp",
        "qmljspropertyinspector.cpp",
        "qmljsinspector.qrc",
    ]
}

