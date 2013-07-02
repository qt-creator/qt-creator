import qbs
import qbs.File

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
    property path qbs_build_dir: qbs.getenv("QBS_BUILD_DIR")
    property path qbs_source_dir: qbs.getenv("QBS_SOURCE_DIR")
    property bool useExternalQbs: qbs_build_dir && qbs_source_dir
    Project {
        name: "qbs"
        id: qbsProject
        property string qbsBaseDir: path + "/shared/qbs"
        condition: qbsSubModuleExists && !useExternalQbs

        property bool enableUnitTests: false
        property bool installApiHeaders: false
        property path libInstallDir: project.ide_library_path
        property path libRPaths:  qbs.targetOS.contains("osx")
            ? ["@loader_path/.."] : ["$ORIGIN/.."]
        property path resourcesInstallDir: project.ide_data_path + "/qbs"

        references: [
            qbsBaseDir + "/src/lib/lib.qbs",
            qbsBaseDir + "/src/plugins/plugins.qbs",
            qbsBaseDir + "/share/share.qbs"
        ]
    }
}
