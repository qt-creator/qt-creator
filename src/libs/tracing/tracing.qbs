QtcLibrary {
    name: "Tracing"

    Depends { name: "Qt"; submodules: ["widgets"] }
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
        name: "Icons"
        Qt.core.resourcePrefix: "qt/qml/QtCreator/Tracing/"
        fileTags: "qt.core.resource_data"
        files: [
            "qml/ico_edit.png", "qml/ico_edit@2x.png",
            "qml/ico_rangeselected.png", "qml/ico_rangeselected@2x.png",
            "qml/ico_rangeselection.png", "qml/ico_rangeselection@2x.png",
            "qml/ico_selectionmode.png", "qml/ico_selectionmode@2x.png",
        ]
    }

    cpp.defines: base.concat("TRACING_LIBRARY")
}
