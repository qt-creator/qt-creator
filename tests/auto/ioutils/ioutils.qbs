import qbs

QtcAutotest {
    name: "IoUtils autotest"
    Depends { name: "Qt.core" }
    files: [
        project.sharedSourcesDir + "/proparser/ioutils.cpp",
        "tst_ioutils.cpp"
    ]
    cpp.includePaths: base.concat([project.sharedSourcesDir])
}
