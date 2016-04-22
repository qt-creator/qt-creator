import qbs

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
}
