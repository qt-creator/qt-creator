import qbs

QtcAutotest {
    name: "QmlProjectManager file format autotest"
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }
    property path fileFormatDir: project.ide_source_tree + "/src/plugins/qmlprojectmanager/fileformat"
    files: "tst_fileformat.cpp"
    Group {
        name: "Files from QmlProjectManager"
        prefix: product.fileFormatDir + '/'
        files: [
            "filefilteritems.cpp",
            "filefilteritems.h",
            "qmlprojectfileformat.cpp",
            "qmlprojectfileformat.h",
            "qmlprojectitem.cpp",
            "qmlprojectitem.h",
        ]
    }
    cpp.includePaths: base.concat([fileFormatDir])
    cpp.defines: base.concat([
        'QT_CREATOR',
        'SRCDIR="' + path + '"'
    ])
}
