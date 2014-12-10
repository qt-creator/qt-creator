import qbs

QtcAutotest {
    Depends { name: "Timeline" }
    Depends { name: "Qt.gui" }

    name: "Timeline Model autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinemodel.cpp"
        ]
    }
}
