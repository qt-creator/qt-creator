import qbs.base 1.0

Project {
    property string ide_version_major: '2'
    property string ide_version_minor: '8'
    property string ide_version_release: '1'
    property string qtcreator_version: ide_version_major + '.' + ide_version_minor + '.' + ide_version_release
    property string ide_app_path: qbs.targetOS.contains("osx") ? "" : "bin"
    property string ide_app_target: qbs.targetOS.contains("osx") ? "Qt Creator" : "qtcreator"
    property string ide_library_path: {
        if (qbs.targetOS.contains("osx"))
            return ide_app_target + ".app/Contents/PlugIns"
        else if (qbs.targetOS.contains("windows"))
            return ide_app_path
        else
            return "lib/qtcreator"
    }
    property string ide_plugin_path: {
        if (qbs.targetOS.contains("osx"))
            return ide_library_path
        else if (qbs.targetOS.contains("windows"))
            return "lib/qtcreator/plugins"
        else
            return ide_library_path + "/plugins"
    }
    property string ide_data_path: qbs.targetOS.contains("osx")
            ? ide_app_target + ".app/Contents/Resources"
            : "share/qtcreator"
    property string ide_libexec_path: qbs.targetOS.contains("osx")
            ? ide_data_path
            : ide_app_path
    property string ide_doc_path: qbs.targetOS.contains("osx")
            ? ide_data_path + "/doc"
            : "share/doc/qtcreator"
    property string ide_bin_path: qbs.targetOS.contains("osx")
            ? ide_app_target + ".app/Contents/MacOS"
            : ide_app_path
    moduleSearchPaths: "qbs"

    references: [
        "src/src.qbs",
        "lib/qtcreator/qtcomponents/qtcomponents.qbs",
        "share/share.qbs",
        "share/qtcreator/translations/translations.qbs",
    ]
}
