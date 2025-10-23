import QmlProject

Project {
    mainFile: "%{ContentDir}/App.qml"
    mainUiFile: "%{ContentDir}/Screen01.ui.qml"

    /* Include .qml, .js, and image files from current directory and subdirectories */
    QmlFiles {
        directory: "%{ProjectName}"
    }

    QmlFiles {
        directory: "%{ContentDir}"
    }

    QmlFiles {
        directory: "%{AssetDir}"
    }

    JavaScriptFiles {
        directory: "%{ProjectName}"
    }

    JavaScriptFiles {
        directory: "%{ContentDir}"
    }

    ImageFiles {
        directory: "%{ContentDir}"
    }

    ImageFiles {
        directory: "%{AssetDir}"
    }

    Files {
       filter: "*.conf"
       files: ["qtquickcontrols2.conf"]
    }

    Files {
        filter: "qmldir"
        directory: "."
    }

    FontFiles {
        filter: "*.ttf;*.otf"
    }

    Files {
        filter: "*.wav;*.mp3"
    }

    Files {
        filter: "*.mp4"
    }

    Files {
        filter: "*.glsl;*.glslv;*.glslf;*.vsh;*.fsh;*.vert;*.frag"
    }

    Files {
        filter: "*.qsb"
    }

    Files {
        filter: "*.json"
    }

    Files {
        filter: "*.mesh"
        directory: "%{AssetDir}"
    }

    Files {
        filter: "*.qad"
        directory: "%{AssetDir}"
    }

    Environment {
       QT_QUICK_CONTROLS_CONF: "qtquickcontrols2.conf"
       QML_COMPAT_RESOLVE_URLS_ON_ASSIGNMENT: "1"
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

    qt6Project: true

    /* List of plugin directories passed to QML runtime */
    importPaths: [ "." ]

    /* Required for deployment */
    targetDirectory: "/opt/%{ProjectName}"

@if %{EnableCMakeGeneration}
    enableCMakeGeneration: true
    standaloneApp: true
@endif

    qdsVersion: "4.8"

    quickVersion: "%{QtQuickVersion}"

    /* If any modules the project imports require widgets (e.g. QtCharts), widgetApp must be true */
    widgetApp: true

    /* args: Specifies command line arguments for qsb tool to generate shaders.
       files: Specifies target files for qsb tool. If path is included, it must be relative to this file.
              Wildcard '*' can be used in the file name part of the path.
              e.g. files: [ "%{ContentDir}/shaders/*.vert", "*.frag" ]  */
    ShaderTool {
        args: "-s --glsl 100es,120,150 --hlsl 50 --msl 12"
        files: [ "%{ContentDir}/shaders/*" ]
    }

    EffectComposer {
        /* Qsb tool arguments for Effect Composer shader generation.
           Default arguments support all pre-built effects.
           Changing qsbArgs may make some or all pre-built effects unusable in the project. */
        qsbArgs: "-s --glsl 300es,140,330,410 --hlsl 50 --msl 12"
    }

    multilanguageSupport: true
    supportedLanguages: ["en"]
    primaryLanguage: "en"

}
