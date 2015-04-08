import qbs

QtcAutotest {
    Depends { name: "Timeline" }
    Depends { name: "Qt.gui" }

    name: "TimelineModelAggregator autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinemodelaggregator.cpp"
        ]
    }
}
