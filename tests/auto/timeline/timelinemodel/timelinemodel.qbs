import qbs

QtcAutotest {
    Depends { name: "Timeline" }
    Depends { name: "Qt.gui" }

    name: "TimelineModel autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinemodel.cpp"
        ]
    }
}
