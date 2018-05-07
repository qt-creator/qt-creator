import qbs 1.0

import QtcLibrary

Project {
    name: "Tracing"

    QtcDevHeaders { }

    QtcLibrary {
        Depends { name: "Qt"; submodules: ["qml", "quick", "gui"] }
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
                "traceevent.h", "traceeventtype.h", "tracestashfile.h"
            ]
        }

        Group {
            name: "QML"
            prefix: "qml/"
            files: ["tracing.qrc"]
        }

        Group {
            name: "Unit test utilities"
            condition: qtc.testsEnabled
            files: [
                "runscenegraphtest.cpp", "runscenegraphtest.h"
            ]
        }

        cpp.defines: base.concat("TRACING_LIBRARY")
    }
}
