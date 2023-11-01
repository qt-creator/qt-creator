import qbs.FileInfo
import qbs.Utilities

Module {
    Depends { name: "qtc" }
    Depends { name: "cpp" }

    Properties {
        condition: qbs.toolchain.contains("gcc")
        cpp.cxxFlags: {
             var flags = ["-Wno-missing-field-initializers"];
             function isClang() { return qbs.toolchain.contains("clang"); }
             function versionAtLeast(v) {
                 return Utilities.versionCompare(cpp.compilerVersion, v) >= 0;
             };
             if (isClang())
                 flags.push("-Wno-constant-logical-operand");
             if ((!isClang() && versionAtLeast("9"))
                     || (isClang() && !qbs.hostOS.contains("darwin") && versionAtLeast("10"))) {
                 flags.push("-Wno-deprecated-copy");
             }
             return flags;
         }
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
    property string pluginsInstallBaseDir: qbs.targetOS.contains("darwin")
                                           ? qtc.ide_plugin_path + "/.."
                                           : qtc.ide_library_path + "/.."
    property string pluginsInstallDir: pluginsInstallBaseDir + "/qbs/plugins"
    property string qmlTypeDescriptionsInstallDir: qtc.ide_data_path + "/qml-type-descriptions"
    property string appInstallDir: qtc.ide_bin_path
    property string libexecInstallDir: qtc.ide_libexec_path
    property bool installHtml: false
    property bool installQch: !qbs.targetOS.contains("macos")
    property string docInstallDir: qtc.ide_doc_path
    property string relativeLibexecPath: FileInfo.relativePath('/' + appInstallDir,
                                                               '/' + libexecInstallDir)
    property string relativePluginsPath: FileInfo.relativePath('/' + appInstallDir,
                                                               '/' + pluginsInstallBaseDir)
    property string relativeSearchPath: FileInfo.relativePath('/' + appInstallDir,
                                                              '/' + resourcesInstallDir)
}
