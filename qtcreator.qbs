import qbs 1.0

Project {
    property bool withAutotests: qbs.buildVariant === "debug"
    property string ide_version_major: '3'
    property string ide_version_minor: '1'
    property string ide_version_release: '83'
    property string qtcreator_version: ide_version_major + '.' + ide_version_minor + '.' + ide_version_release
    property string ide_compat_version_major: '3'
    property string ide_compat_version_minor: '1'
    property string ide_compat_version_release: '83'
    property string qtcreator_compat_version: ide_compat_version_major + '.' + ide_compat_version_minor + '.' + ide_compat_version_release
    property path ide_source_tree: path
    property string ide_app_path: qbs.targetOS.contains("osx") ? "" : "bin"
    property string ide_app_target: qbs.targetOS.contains("osx") ? "Qt Creator" : "qtcreator"
    property pathList additionalPlugins: []
    property pathList additionalLibs: []
    property pathList additionalTools: []
    property string libDirName: "lib"
    property string ide_library_path: {
        if (qbs.targetOS.contains("osx"))
            return ide_app_target + ".app/Contents/PlugIns"
        else if (qbs.targetOS.contains("windows"))
            return ide_app_path
        else
            return libDirName + "/qtcreator"
    }
    property string ide_plugin_path: {
        if (qbs.targetOS.contains("osx"))
            return ide_library_path
        else if (qbs.targetOS.contains("windows"))
            return libDirName + "/qtcreator/plugins"
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
    property bool testsEnabled: qbs.getEnv("TEST") || qbs.buildVariant === "debug"
    property stringList generalDefines: [
        "QT_CREATOR",
        'IDE_LIBRARY_BASENAME="' + libDirName + '"',
        "QT_DISABLE_DEPRECATED_BEFORE=0x040900",
        "QT_NO_CAST_TO_ASCII",
        "QT_NO_CAST_FROM_ASCII"
    ].concat(testsEnabled ? ["WITH_TESTS"] : [])
    qbsSearchPaths: "qbs"

    references: [
        "src/src.qbs",
        "share/share.qbs",
        "share/qtcreator/translations/translations.qbs",
        "tests/tests.qbs"
    ]
}
