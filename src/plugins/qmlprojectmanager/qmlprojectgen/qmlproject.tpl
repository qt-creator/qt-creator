import QmlProject

Project {

    /* Include .qml, .js, and image files from current directory and subdirectories */

%1
%2
%3
%4

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
        filter: "*.wav;*.mp3"
    }

    Files {
        filter: "*.mp4"
    }

    Files {
        filter: "*.glsl;*.glslv;*.glslf;*.vsh;*.fsh;*.vert;*.frag"
    }

    Environment {
        QT_QUICK_CONTROLS_CONF: "qtquickcontrols2.conf"
        QT_AUTO_SCREEN_SCALE_FACTOR: "1"
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
    createdInQtCreator: true
    qdsVersion: "3.4"

    %5

    /* If any modules the project imports require widgets (e.g. QtCharts), widgetApp must be true */
    widgetApp: true

    /* args: Specifies command line arguments for qsb tool to generate shaders.
       files: Specifies target files for qsb tool. If path is included, it must be relative to this file.
              Wildcard '*' can be used in the file name part of the path.
              e.g. files: [ "content/shaders/*.vert", "*.frag" ]  */
    ShaderTool {
        args: "-s --glsl \\\"100 es,120,150\\\" --hlsl 50 --msl 12"
        files: [ "content/shaders/*" ]
    }

    multilanguageSupport: true
    supportedLanguages: ["en"]
    primaryLanguage: "en"
}
