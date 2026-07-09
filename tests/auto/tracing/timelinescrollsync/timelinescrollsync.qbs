import qbs

QtcAutotest {
    name: "TimelineScrollSync autotest"

    Depends { name: "Tracing"; required: false }
    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.gui" }

    condition: Tracing.present

    Group {
        name: "Test sources"
        files: ["tst_timelinescrollsync.cpp"]
    }
}
