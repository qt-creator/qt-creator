import qbs.FileInfo
import qbs.Utilities

Module {
    Depends { name: "qtc" }
    Depends { name: "cpp" }

    Properties {
        condition: qbs.toolchain.contains("gcc") && !qbs.toolchain.contains("clang")
                   && Utilities.versionCompare(cpp.compilerVersion, "9") >= 0
        cpp.cxxFlags: ["-Wno-deprecated-copy", "-Wno-init-list-lifetime"]
    }

    Properties {
        condition: qbs.toolchain.contains("clang") && !qbs.hostOS.contains("darwin")
                   && Utilities.versionCompare(cpp.compilerVersion, "10") >= 0
        cpp.cxxFlags: ["-Wno-deprecated-copy", "-Wno-constant-logical-operand"]
    }

    priority: 1

    property bool enableUnitTests: false
    property bool enableProjectFileUpdates: true
    property bool installApiHeaders: false
    property bool enableBundledQt: false
    property string libInstallDir: qtc.ide_library_path
    property stringList libRPaths: qbs.targetOS.contains("macos")
            ? ["@loader_path/" + FileInfo.relativePath('/' + appInstallDir, '/' + libInstallDir)]
            : ["$ORIGIN/..", "$ORIGIN/../" + qtc.ide_library_path]
    property string resourcesInstallDir: qtc.ide_data_path + "/qbs"
    property string pluginsInstallDir: qtc.ide_plugin_path + "/qbs/plugins"
    property string qmlTypeDescriptionsInstallDir: qtc.ide_data_path + "/qml-type-descriptions"
    property string appInstallDir: qtc.ide_bin_path
    property string libexecInstallDir: qtc.ide_libexec_path
    property bool installHtml: false
    property bool installQch: !qbs.targetOS.contains("macos")
    property string docInstallDir: qtc.ide_doc_path
    property string relativeLibexecPath: FileInfo.relativePath('/' + appInstallDir,
                                                               '/' + libexecInstallDir)
    property string relativePluginsPath: FileInfo.relativePath('/' + appInstallDir,
                                                               '/' + qtc.ide_plugin_path)
    property string relativeSearchPath: FileInfo.relativePath('/' + appInstallDir,
                                                              '/' + resourcesInstallDir)
}
