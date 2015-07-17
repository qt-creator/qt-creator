import qbs
import qbs.File
import qbs.FileInfo

Project {
    name: "Sources"
    references: [
        "app/app.qbs",
        "app/app_version_header.qbs",
        "libs/libs.qbs",
        "plugins/plugins.qbs",
        "tools/tools.qbs"
    ]

    property bool qbsSubModuleExists: File.exists(qbsProject.qbsBaseDir + "/qbs.qbs")
    property path qbs_install_dir: qbs.getEnv("QBS_INSTALL_DIR")
    property bool useExternalQbs: qbs_install_dir
    property bool buildQbsProjectManager: useExternalQbs || qbsSubModuleExists
    Project {
        name: "qbs"
        id: qbsProject
        property string qbsBaseDir: project.sharedSourcesDir + "/qbs"
        condition: qbsSubModuleExists && !useExternalQbs
        qbsSearchPaths: [qbsBaseDir + "/qbs-resources"]

        property bool enableUnitTests: false
        property bool enableProjectFileUpdates: true
        property bool installApiHeaders: false
        property string libInstallDir: project.ide_library_path
        property stringList libRPaths:  qbs.targetOS.contains("osx")
            ? ["@loader_path/" + FileInfo.relativePath(appInstallDir, libInstallDir)]
            : ["$ORIGIN/..", "$ORIGIN/../" + project.ide_library_path]
        property string resourcesInstallDir: project.ide_data_path + "/qbs"
        property string pluginsInstallDir: project.ide_plugin_path
        property string appInstallDir: project.ide_bin_path
        property string libexecInstallDir: project.ide_libexec_path
        property string relativeLibexecPath: FileInfo.relativePath(appInstallDir, libexecInstallDir)
        property string relativePluginsPath: FileInfo.relativePath(appInstallDir, pluginsInstallDir)
        property string relativeSearchPath: FileInfo.relativePath(appInstallDir,
                                                                  resourcesInstallDir)

        references: [
            qbsBaseDir + "/src/lib/libs.qbs",
            qbsBaseDir + "/src/libexec/libexec.qbs",
            qbsBaseDir + "/src/plugins/plugins.qbs",
            qbsBaseDir + "/share/share.qbs",
            qbsBaseDir + "/src/app/apps.qbs",
        ]
    }
}
