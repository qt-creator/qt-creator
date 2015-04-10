import qbs

QtcAutotest {
    Depends { name: "Timeline" }
    Depends { name: "Qt.gui" }

    name: "TimelineNotesModel autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinenotesmodel.cpp"
        ]
    }
}
