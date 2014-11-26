import qbs

QtcPlugin {
    name: "QmlProfilerExtension"

    Depends { name: "Core" }
    Depends { name: "LicenseChecker" }
    Depends { name: "QmlProfiler" }

    Depends { name: "Qt.widgets" }

    files: [
        "inputeventsmodel.cpp",
        "inputeventsmodel.h",
        "memoryusagemodel.cpp",
        "memoryusagemodel.h",
        "pixmapcachemodel.cpp",
        "pixmapcachemodel.h",
        "qmlprofilerextensionconstants.h",
        "qmlprofilerextensionplugin.cpp",
        "qmlprofilerextensionplugin.h",
        "qmlprofilerextension_global.h",
        "scenegraphtimelinemodel.cpp",
        "scenegraphtimelinemodel.h",
    ]
}
