import qbs

Product {
    builtByDefault: false
    type: [isOnlineDoc ? "qdoc-output" : "qch"]
    Depends { name: "Qt.core" }

    property path mainDocConfFile
    property bool isOnlineDoc

    Group {
        name: "main qdocconf file"
        files: [mainDocConfFile]
        fileTags: ["qdocconf-main"]
    }

    property string versionTag: project.qtcreator_version.replace(/\.|-/g, "")
    Qt.core.qdocEnvironment: [
        "QTC_LICENSE_TYPE=" + project.licenseType,
        "QTC_VERSION=" + project.qtcreator_version,
        "QTC_VERSION_TAG=" + project.qtcreator_version,
        "SRCDIR=" + sourceDirectory,
        "QT_INSTALL_DOCS=" + Qt.core.docPath,
        "QDOC_INDEX_DIR=" + Qt.core.docPath,
        "VERSION_TAG=" + versionTag
    ]

    Group {
        fileTagsFilter: ["qch"]
        qbs.install: !qbs.targetOS.contains("osx")
        qbs.installDir: project.ide_doc_path
    }
}
