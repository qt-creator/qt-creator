import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "QmlEditorWidgets"

    cpp.includePaths: [
        ".",
        "..",
        "easingpane"
    ]
    cpp.defines: base.concat([
        "QWEAKPOINTER_ENABLE_ARROW",
        "BUILD_QMLEDITORWIDGETS_LIB",
        "QT_CREATOR"
    ])
    cpp.optimization: "fast"

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["widgets", "quick1", "script"] }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    files: [
        "resources.qrc",
        "fontsizespinbox.h",
        "filewidget.h",
        "contextpanewidgetrectangle.h",
        "contextpanewidgetimage.h",
        "contextpanewidget.h",
        "contextpanetextwidget.h",
        "colorwidgets.h",
        "colorbutton.h",
        "colorbox.h",
        "customcolordialog.h",
        "gradientline.h",
        "huecontrol.h",
        "qmleditorwidgets_global.h",
        "fontsizespinbox.cpp",
        "filewidget.cpp",
        "contextpanewidgetrectangle.cpp",
        "contextpanewidgetimage.cpp",
        "contextpanewidget.cpp",
        "contextpanetextwidget.cpp",
        "colorwidgets.cpp",
        "colorbox.cpp",
        "customcolordialog.cpp",
        "huecontrol.cpp",
        "gradientline.cpp",
        "colorbutton.cpp",
        "contextpanewidgetrectangle.ui",
        "contextpanewidgetimage.ui",
        "contextpanewidgetborderimage.ui",
        "contextpanetext.ui",
        "easingpane/easinggraph.cpp",
        "easingpane/easingcontextpane.cpp",
        "easingpane/easinggraph.h",
        "easingpane/easingcontextpane.h",
        "easingpane/easingpane.qrc",
        "easingpane/easingcontextpane.ui"
    ]

    ProductModule {
        Depends { name: "cpp" }
        cpp.includePaths: ["."]
    }
}

