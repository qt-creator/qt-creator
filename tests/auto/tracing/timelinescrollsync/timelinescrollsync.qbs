import qbs

QtcAutotest {
    name: "TimelineScrollSync autotest"

    Depends { name: "Tracing" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.gui" }

    Group {
        name: "Test sources"
        files: ["tst_timelinescrollsync.cpp"]
    }
}
