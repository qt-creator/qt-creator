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
            "contextpanetextwidget.cpp", "contextpanetextwidget.h",
            "contextpanewidget.cpp", "contextpanewidget.h",
            "contextpanewidgetimage.cpp", "contextpanewidgetimage.h",
            "contextpanewidgetrectangle.cpp", "contextpanewidgetrectangle.h",
            "customcolordialog.cpp", "customcolordialog.h",
            "filewidget.cpp", "filewidget.h",
            "fontsizespinbox.cpp", "fontsizespinbox.h",
            "gradientline.cpp", "gradientline.h",
            "huecontrol.cpp", "huecontrol.h",
            "qmleditorwidgets_global.h",
            "qmleditorwidgetstr.h",
            "resources_qmleditorwidgets.qrc"
        ]
    }

    Group {
        name: "Easing Pane"
        id: easingPane
        prefix: "easingpane/"
        files: [
            "easingcontextpane.cpp", "easingcontextpane.h",
            "easinggraph.cpp", "easinggraph.h",
            "easingpane.qrc",
        ]
    }
}
