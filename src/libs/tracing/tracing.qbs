QtcLibrary {
    name: "Tracing"

    Depends { name: "Qt"; submodules: ["widgets", "canvaspainter"] }
    Depends { name: "Qt.testlib"; condition: qtc.withAutotests }
    Depends { name: "Utils" }

    files: [
        "flamegraphwidget.cpp", "flamegraphwidget.h",
        "safecastable.h",
        "timelinecoordinates.h",
        "timelineformattime.cpp", "timelineformattime.h",
        "trackpainter.cpp", "trackpainter.h",
        "tracklabels.cpp", "tracklabels.h",
        "timeruler.cpp", "timeruler.h",
        "rangedetailswidget.cpp", "rangedetailswidget.h",
        "selectionrangedetailswidget.cpp", "selectionrangedetailswidget.h",
        "selectionrangeoverlay.cpp", "selectionrangeoverlay.h",
        "timelinescrollsync.cpp", "timelinescrollsync.h",
        "timelinecontentwidget.cpp", "timelinecontentwidget.h",
        "timelineoverviewwidget.cpp", "timelineoverviewwidget.h",
        "timelinemodel.cpp", "timelinemodel.h", "timelinemodel_p.h",
        "timelinemodelaggregator.cpp", "timelinemodelaggregator.h",
        "timelinenotesmodel.cpp", "timelinenotesmodel.h",
        "timelinetracefile.cpp", "timelinetracefile.h",
        "timelinetracemanager.cpp", "timelinetracemanager.h",
        "timelinewidget.cpp", "timelinewidget.h",
        "timelinezoomcontrol.cpp", "timelinezoomcontrol.h",
        "traceevent.h", "traceeventtype.h", "tracestashfile.h",
        "tracingtr.h",
    ]

    Group {
        name: "images"
        prefix: "images/"
        fileTags: "qt.core.resource_data"
        Qt.core.resourceSourceBase: sourceDirectory + "/.."
        files: [
            "edit.png", "edit@2x.png",
            "rangeselected.png", "rangeselected@2x.png",
            "rangeselection.png", "rangeselection@2x.png",
            "selectionmode.png", "selectionmode@2x.png",
        ]
    }

    cpp.defines: base.concat("TRACING_LIBRARY")
}
