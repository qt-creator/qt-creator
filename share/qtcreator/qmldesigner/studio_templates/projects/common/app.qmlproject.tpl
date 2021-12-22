@if %{IsQt6Project}
import QmlProject
@else
import QmlProject 1.1
@endif

Project {
    mainFile: "content/App.qml"

    /* Include .qml, .js, and image files from current directory and subdirectories */
    QmlFiles {
        directory: "."
    }

    JavaScriptFiles {
        directory: "."
    }

    ImageFiles {
        directory: "."
    }

    Files {
       filter: "*.conf"
       files: ["qtquickcontrols2.conf"]
    }

    Files {
        filter: "qmldir"
        directory: "."
    }

    Files {
        filter: "*.ttf;*.otf"
    }

    Files {
        filter: "*.mesh"
        directory: "asset_imports"
    }

    Environment {
       QT_QUICK_CONTROLS_CONF: "qtquickcontrols2.conf"
       QT_AUTO_SCREEN_SCALE_FACTOR: "1"
@if %{UseVirtualKeyboard}
       QT_IM_MODULE: "qtvirtualkeyboard"
       QT_VIRTUALKEYBOARD_DESKTOP_DISABLE: 1
@endif
       QT_LOGGING_RULES: "qt.qml.connections=false"
       QT_ENABLE_HIGHDPI_SCALING: "0"
       /* Useful for debugging
       QSG_VISUALIZE=batches
       QSG_VISUALIZE=clip
       QSG_VISUALIZE=changes
       QSG_VISUALIZE=overdraw
       */
    }

@if %{IsQt6Project}
    qt6Project: true
@endif

    /* List of plugin directories passed to QML runtime */
    importPaths: [ ".", "imports", "asset_imports" ]

    /* Required for deployment */
    targetDirectory: "/opt/%{ProjectName}"

@if %{IsQt6Project}
    /* If any modules the project imports require widgets (e.g. QtCharts), widgetApp must be true */
    widgetApp: true
@endif
}
