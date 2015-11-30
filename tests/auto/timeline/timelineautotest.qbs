import qbs

QtcAutotest {
    Depends { name: "Timeline" }
    Depends { name: "Qt.quick" }
    Depends { name: "Qt.gui" }

    Group {
        name: "Shared sources"
        files: [
            "../shared/runscenegraph.h",
            "../shared/runscenegraph.cpp"
        ]
    }
}
