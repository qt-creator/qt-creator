import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "QmlEditorWidgets"

    cpp.includePaths: base.concat("easingpane")
    cpp.defines: base.concat([
        "QWEAKPOINTER_ENABLE_ARROW",
        "BUILD_QMLEDITORWIDGETS_LIB",
        "QT_CREATOR"
    ])
    cpp.optimization: "fast"

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["widgets", "declarative", "script"] }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    files: [
        "colorbox.cpp",
        "colorbox.h",
        "colorbutton.cpp",
        "colorbutton.h",
        "colorwidgets.cpp",
        "colorwidgets.h",
        "contextpanetext.ui",
        "contextpanetextwidget.cpp",
        "contextpanetextwidget.h",
        "contextpanewidget.cpp",
        "contextpanewidget.h",
        "contextpanewidgetborderimage.ui",
        "contextpanewidgetimage.cpp",
        "contextpanewidgetimage.h",
        "contextpanewidgetimage.ui",
        "contextpanewidgetrectangle.cpp",
        "contextpanewidgetrectangle.h",
        "contextpanewidgetrectangle.ui",
        "customcolordialog.cpp",
        "customcolordialog.h",
        "filewidget.cpp",
        "filewidget.h",
        "fontsizespinbox.cpp",
        "fontsizespinbox.h",
        "gradientline.cpp",
        "gradientline.h",
        "huecontrol.cpp",
        "huecontrol.h",
        "qmleditorwidgets_global.h",
        "resources.qrc",
        "easingpane/easingcontextpane.cpp",
        "easingpane/easingcontextpane.h",
        "easingpane/easingcontextpane.ui",
        "easingpane/easinggraph.cpp",
        "easingpane/easinggraph.h",
        "easingpane/easingpane.qrc",
    ]
}

