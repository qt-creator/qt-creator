import qbs
import qbs.Environment
import qbs.File
import qbs.FileInfo

Project {
    name: "Sources"
    references: [
        "app/app.qbs",
        "app/app_version_header.qbs",
        "libs/libs.qbs",
        "plugins/plugins.qbs",
        "tools/tools.qbs",
        project.sharedSourcesDir + "/json",
        project.sharedSourcesDir + "/proparser",
        project.sharedSourcesDir + "/pch_files.qbs",
    ]

    property bool qbsSubModuleExists: File.exists(qbsProject.qbsBaseDir + "/qbs.qbs")
    property path qbs_install_dir: Environment.getEnv("QBS_INSTALL_DIR")
    property bool useExternalQbs: qbs_install_dir
    property bool buildQbsProjectManager: useExternalQbs || qbsSubModuleExists
    Project {
        name: "qbs project"
        id: qbsProject
        property string qbsBaseDir: project.sharedSourcesDir + "/qbs"
        condition: qbsSubModuleExists && !useExternalQbs

        // The first entry is for overriding qbs' own qbsbuildconfig module.
        qbsSearchPaths: [project.ide_source_tree + "/qbs", qbsBaseDir + "/qbs-resources"]

        references: [
            qbsBaseDir + "/src/lib/libs.qbs",
            qbsBaseDir + "/src/libexec/libexec.qbs",
            qbsBaseDir + "/src/plugins/plugins.qbs",
            qbsBaseDir + "/share/share.qbs",
            qbsBaseDir + "/src/app/apps.qbs",
            qbsBaseDir + "/src/shared/json/json.qbs",
        ]
    }
}
