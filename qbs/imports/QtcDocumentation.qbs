import qbs
import qbs.FileInfo

Product {
    builtByDefault: false
    type: [isOnlineDoc ? "qdoc-output" : "qch"]
    Depends { name: "Qt.core" }
    Depends { name: "qtc" }

    property path mainDocConfFile
    property bool isOnlineDoc

    Group {
        name: "main qdocconf file"
        prefix: product.sourceDirectory + '/'
        files: [mainDocConfFile]
        fileTags: ["qdocconf-main"]
    }

    property string versionTag: qtc.qtcreator_version.replace(/\.|-/g, "")
    Qt.core.qdocEnvironment: [
        "QTC_LICENSE_TYPE=" + project.licenseType,
        "QTC_VERSION=" + qtc.qtcreator_version,
        "QTC_VERSION_TAG=" + qtc.qtcreator_version,
        "SRCDIR=" + sourceDirectory,
        "QT_INSTALL_DOCS=" + Qt.core.docPath,
        "QDOC_INDEX_DIR=" + Qt.core.docPath,
        "VERSION_TAG=" + versionTag
    ]

    Group {
        fileTagsFilter: ["qch"]
        qbs.install: !qbs.targetOS.contains("macos")
        qbs.installDir: qtc.ide_doc_path
    }
}
