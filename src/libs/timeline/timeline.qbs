import qbs 1.0

import QtcLibrary

QtcLibrary {
    name: "Timeline"

    Depends { name: "Qt"; submodules: ["qml", "quick", "gui"] }
    Depends { name: "Utils" }

    Group {
        name: "General"
        files: [
            "README",
            "timelineabstractrenderer.cpp", "timelineabstractrenderer.h",
            "timelineabstractrenderer_p.h",
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
            "timelinezoomcontrol.cpp", "timelinezoomcontrol.h"
        ]
    }

    Group {
        name: "QML"
        prefix: "qml/"
        files: ["timeline.qrc"]
    }

    cpp.defines: base.concat("TIMELINE_LIBRARY")
}
