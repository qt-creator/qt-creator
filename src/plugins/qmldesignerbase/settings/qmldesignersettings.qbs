QtcLibrary {
    name: "QmlDesignerSettings"
    type: "staticlibrary"

    Depends { name: "Utils" }

    cpp.defines: base.concat("QMLDESIGNERSETTINGS_STATIC_LIBRARY")

    files: [
        "qmldesignersettings_global.h",
        "designersettings.cpp",
        "designersettings.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.defines: base.concat("QMLDESIGNERSETTINGS_STATIC_LIBRARY")
    }
}
