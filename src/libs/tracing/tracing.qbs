QtcLibrary {
    name: "Tracing"

    Depends { name: "Qt"; submodules: ["qml", "quick", "gui"] }
    Depends { name: "Qt.testlib"; condition: qtc.withAutotests }
    Depends { name: "Utils" }

    Group {
        name: "General"
        files: [
            "README",
            "flamegraph.cpp", "flamegraph.h",
            "flamegraphattached.h",
            "safecastable.h",
            "timelineabstractrenderer.cpp", "timelineabstractrenderer.h",
            "timelineabstractrenderer_p.h",
            "timelineformattime.cpp", "timelineformattime.h",
            "timelineitemsrenderpass.cpp", "timelineitemsrenderpass.h",
            "timelinemodel.cpp", "timelinemodel.h", "timelinemodel_p.h",
            "timelinemodelaggregator.cpp", "timelinemodelaggregator.h",
            "timelinenotesmodel.cpp", "timelinenotesmodel.h", "timelinenotesmodel_p.h",
            "timelinenotesrenderpass.cpp", "timelinenotesrenderpass.h",
            "timelineoverviewrenderer.cpp", "timelineoverviewrenderer.h",
            "timelineoverviewrenderer_p.h",
            "timelinerenderer.cpp", "timelinerenderer.h", "timelinerenderer_p.h",
            "timelinerenderpass.cpp", "timelinerenderpass.h",
            "timelinerenderstate.cpp", "timelinerenderstate.h", "timelinerenderstate_p.h",
            "timelineselectionrenderpass.cpp", "timelineselectionrenderpass.h",
            "timelinetheme.cpp", "timelinetheme.h",
            "timelinetracefile.cpp", "timelinetracefile.h",
            "timelinetracemanager.cpp", "timelinetracemanager.h",
            "timelinezoomcontrol.cpp", "timelinezoomcontrol.h",
            "traceevent.h", "traceeventtype.h", "tracestashfile.h",
            "tracingtr.h",
        ]
    }

    Group {
        name: "Qml Files"
        Qt.core.resourcePrefix: "qt/qml/QtCreator/Tracing/"
        fileTags: "qt.core.resource_data"
        files: "qml/**"
    }

    cpp.defines: base.concat("TRACING_LIBRARY")
}
