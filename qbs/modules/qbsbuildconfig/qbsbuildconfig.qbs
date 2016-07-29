import qbs
import qbs.FileInfo

Module {
    Depends { name: "qtc" }

    property bool enableUnitTests: false
    property bool enableProjectFileUpdates: true
    property bool installApiHeaders: false
    property string libInstallDir: qtc.ide_library_path
    property stringList libRPaths: qbs.targetOS.contains("macos")
            ? ["@loader_path/" + FileInfo.relativePath('/' + appInstallDir, '/' + libInstallDir)]
            : ["$ORIGIN/..", "$ORIGIN/../" + qtc.ide_library_path]
    property string resourcesInstallDir: qtc.ide_data_path + "/qbs"
    property string pluginsInstallDir: qtc.ide_plugin_path
    property string appInstallDir: qtc.ide_bin_path
    property string libexecInstallDir: qtc.ide_libexec_path
    property string relativeLibexecPath: FileInfo.relativePath('/' + appInstallDir,
                                                               '/' + libexecInstallDir)
    property string relativePluginsPath: FileInfo.relativePath('/' + appInstallDir,
                                                               '/' + pluginsInstallDir)
    property string relativeSearchPath: FileInfo.relativePath('/' + appInstallDir,
                                                              '/' + resourcesInstallDir)
}
