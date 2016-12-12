import qbs 1.0

QtcLibrary {
    name: "QmlEditorWidgets"

    cpp.defines: base.concat([
        "QMLEDITORWIDGETS_LIBRARY"
    ])
    cpp.optimization: "fast"

    Depends { name: "Qt"; submodules: ["widgets"] }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Group {
        name: "General"
        files: [
            "colorbox.cpp", "colorbox.h",
            "colorbutton.cpp", "colorbutton.h",
            "contextpanetext.ui",
            "contextpanetextwidget.cpp", "contextpanetextwidget.h",
            "contextpanewidget.cpp", "contextpanewidget.h",
            "contextpanewidgetborderimage.ui",
            "contextpanewidgetimage.cpp", "contextpanewidgetimage.h", "contextpanewidgetimage.ui",
            "contextpanewidgetrectangle.cpp", "contextpanewidgetrectangle.h", "contextpanewidgetrectangle.ui",
            "customcolordialog.cpp", "customcolordialog.h",
            "filewidget.cpp", "filewidget.h",
            "fontsizespinbox.cpp", "fontsizespinbox.h",
            "gradientline.cpp", "gradientline.h",
            "huecontrol.cpp", "huecontrol.h",
            "qmleditorwidgets_global.h",
            "resources.qrc"
        ]
    }

    Group {
        name: "Easing Pane"
        id: easingPane
        prefix: "easingpane/"
        files: [
            "easingcontextpane.cpp", "easingcontextpane.h", "easingcontextpane.ui",
            "easinggraph.cpp", "easinggraph.h",
            "easingpane.qrc",
        ]
    }
}
