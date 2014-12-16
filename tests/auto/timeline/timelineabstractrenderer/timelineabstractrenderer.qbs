import qbs

QtcAutotest {
    Depends { name: "Timeline" }
    Depends { name: "Qt.quick" }

    name: "TimelineAbstractRenderer autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelineabstractrenderer.cpp"
        ]
    }
}
