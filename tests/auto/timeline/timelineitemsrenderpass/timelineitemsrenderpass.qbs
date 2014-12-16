import qbs
import QtcFunctions

QtcAutotest {
    condition: QtcFunctions.versionIsAtLeast(Qt.core.version, "5.4")
    Depends { name: "Timeline" }
    Depends { name: "Qt.quick" }

    name: "TimelineItemsRenderPass autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelineitemsrenderpass.cpp"
        ]
    }
}
