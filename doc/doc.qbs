import qbs
import qbs.File

Project {
    name: "documentation"

    QtcDocumentation {
        name: "user doc online"
        isOnlineDoc: true
        mainDocConfFile: "qtcreator-online.qdocconf"

        files: [
            "src/**/*",
        ]
    }

    QtcDocumentation {
        name: "user doc offline"
        isOnlineDoc: false
        mainDocConfFile: "qtcreator.qdocconf"

        files: [
            "src/**/*",
        ]
    }

    QtcDocumentation {
        name: "API doc online"
        isOnlineDoc: true
        mainDocConfFile: "api/qtcreator-dev-online.qdocconf"

        Group {
            name: "sources"
            files: [
                "api/*.qdoc",
                "api/**/*",
            ]
            excludeFiles: [mainDocConfFile]
        }
    }

    QtcDocumentation {
        name: "API doc offline"
        isOnlineDoc: false
        mainDocConfFile: "api/qtcreator-dev.qdocconf"

        Group {
            name: "sources"
            files: [
                "api/*.qdoc",
                "api/**/*",
            ]
            excludeFiles: [mainDocConfFile]
        }
    }

    property string qbsBaseDir: project.sharedSourcesDir + "/qbs"
    property bool qbsSubModuleExists: File.exists(qbsBaseDir + "/qbs.qbs")
    Properties {
        condition: qbsSubModuleExists

        references: [qbsBaseDir + "/doc/doc.qbs"]

        // The first entry is for overriding qbs' own qbsbuildconfig module.
        qbsSearchPaths: [project.ide_source_tree + "/qbs", qbsBaseDir + "/qbs-resources"]
    }
}
