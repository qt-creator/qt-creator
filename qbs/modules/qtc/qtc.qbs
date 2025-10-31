import qbs
import qbs.Environment
import qbs.FileInfo
import qbs.Utilities

Module {
    Depends { name: "cpp" }

    property string qtcreator_display_version: '18.0.1'
    property string ide_version_major: '18'
    property string ide_version_minor: '0'
    property string ide_version_release: '1'
    property string qtcreator_version: ide_version_major + '.' + ide_version_minor + '.'
                                       + ide_version_release

    property string ide_compat_version_major: '18'
    property string ide_compat_version_minor: '0'
    property string ide_compat_version_release: '0'
    property string qtcreator_compat_version: ide_compat_version_major + '.'
            + ide_compat_version_minor + '.' + ide_compat_version_release

    property string ide_author: "The Qt Company Ltd. and other contributors."
    property string ide_copyright_string: "Copyright (C) The Qt Company Ltd. and other contributors."

    property string ide_display_name: 'Qt Creator'
    property string ide_id: 'qtcreator'
    property string ide_cased_id: 'QtCreator'
    property string ide_bundle_identifier: 'org.qt-project.qtcreator'
    property string ide_user_file_extension: '.user'

    property string libDirName: "lib"
    property string ide_app_path: qbs.targetOS.contains("macos") ? "" : "bin"
    property string ide_app_target: qbs.targetOS.contains("macos") ? ide_display_name : ide_id
    property string ide_library_path: {
        if (qbs.targetOS.contains("macos"))
            return ide_app_target + ".app/Contents/Frameworks"
        else if (qbs.targetOS.contains("windows"))
            return ide_app_path
        else
            return libDirName + "/qtcreator"
    }
    property string ide_plugin_path: {
        if (qbs.targetOS.contains("macos"))
            return ide_app_target + ".app/Contents/PlugIns/qtcreator"
        else if (qbs.targetOS.contains("windows"))
            return libDirName + "/qtcreator/plugins"
        else
            return ide_library_path + "/plugins"
    }
    property string ide_data_path: qbs.targetOS.contains("macos")
            ? ide_app_target + ".app/Contents/Resources"
            : "share/qtcreator"
    property string ide_libexec_path: qbs.targetOS.contains("macos")
            ? ide_data_path + "/libexec" : qbs.targetOS.contains("windows")
            ? ide_app_path
            : "libexec/qtcreator"
    property string ide_bin_path: qbs.targetOS.contains("macos")
            ? ide_app_target + ".app/Contents/MacOS"
            : ide_app_path
    property string ide_doc_path: qbs.targetOS.contains("macos")
            ? ide_data_path + "/doc"
            : "share/doc/qtcreator"
    property string ide_include_path: "include"
    property string ide_qbs_resources_path: "qbs-resources"
    property string ide_qbs_modules_path: ide_qbs_resources_path + "/modules"
    property string ide_qbs_imports_path: ide_qbs_resources_path + "/imports"
    property string ide_shared_sources_path: "src/shared"

    property string litehtmlInstallDir: Environment.getEnv("LITEHTML_INSTALL_DIR")

    property bool enableAddressSanitizer: false
    property bool enableUbSanitizer: false
    property bool enableThreadSanitizer: false

    property bool preferSystemSyntaxHighlighting: true

    property bool withAllTests: Environment.getEnv("TEST") || qbs.buildVariant === "debug"
    property bool withPluginTests: withAllTests
    property bool withAutotests: withAllTests

    property stringList generalDefines: [
        "QT_CREATOR",
        'IDE_APP_ID="org.qt-project.qtcreator"',
        'IDE_LIBRARY_BASENAME="' + libDirName + '"',
        'RELATIVE_PLUGIN_PATH="' + FileInfo.relativePath('/' + ide_bin_path,
                                                         '/' + ide_plugin_path) + '"',
        'RELATIVE_LIBEXEC_PATH="' + FileInfo.relativePath('/' + ide_bin_path,
                                                          '/' + ide_libexec_path) + '"',
        'RELATIVE_DATA_PATH="' + FileInfo.relativePath('/' + ide_bin_path,
                                                       '/' + ide_data_path) + '"',
        'RELATIVE_DOC_PATH="' + FileInfo.relativePath('/' + ide_bin_path, '/' + ide_doc_path) + '"',
        "QT_NO_CAST_TO_ASCII",
        "QT_RESTRICTED_CAST_FROM_ASCII",
        "QT_NO_FOREACH",
        "QT_DISABLE_DEPRECATED_BEFORE=0x050900",
        "QT_WARN_DEPRECATED_BEFORE=0x060400",
        "QT_WARN_DEPRECATED_UP_TO=0x060400",
        "QT_USE_QSTRINGBUILDER",
        "QT_NO_QSNPRINTF",
    ].concat(withPluginTests ? ["WITH_TESTS"] : [])
     .concat(qbs.toolchain.contains("msvc") ? ["_CRT_SECURE_NO_WARNINGS"] : [])

    validate: {
        if (qbs.toolchain.contains("msvc") && Utilities.versionCompare(cpp.compilerVersion, "19.30.0") < 0)
            throw "You need at least MSVC2022 to build Qt Creator.";
    }
}
