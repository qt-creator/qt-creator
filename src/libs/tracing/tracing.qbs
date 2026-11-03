QtcLibrary {
    name: "Tracing"

    // The GPU track backend needs CanvasPainter; without it, and on WebAssembly,
    // only the software one is built (see the CMakeLists.txt).
    property bool withGpuBackend: Qt.canvaspainter.present
                                  && !qbs.toolchain.contains("emscripten")

    Depends { name: "Qt"; submodules: ["widgets"] }
    Depends { name: "Qt.canvaspainter"; required: false }
    Depends { name: "Qt.testlib"; condition: qtc.withAutotests }
    Depends { name: "Utils" }

    files: [
        "flamegraphwidget.cpp", "flamegraphwidget.h",
        "safecastable.h",
        "timelinecoordinates.h",
        "timelineformatdata.cpp", "timelineformatdata.h",
        "trackpainterbase.cpp", "trackpainterbase.h",
        "trackpainterraster.cpp", "trackpainterraster.h",
        "tracklabels.cpp", "tracklabels.h",
        "timeruler.cpp", "timeruler.h",
        "timelinedetailswidget.cpp", "timelinedetailswidget.h",
        "pausedrangesoverlay.cpp", "pausedrangesoverlay.h",
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
        name: "GPU track backend"
        condition: product.withGpuBackend
        files: ["trackpaintergpu.cpp", "trackpaintergpu.h"]
    }

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

    cpp.defines: {
        var defines = base.concat("TRACING_LIBRARY");
        if (withGpuBackend)
            defines.push("TRACING_WITH_GPU_BACKEND");
        return defines;
    }

    Export {
        Depends { name: "Utils" }
        Depends {
            name: "Qt.canvaspainter"
            condition: exportingProduct.withGpuBackend
        }
        Depends { name: "cpp" }
        cpp.defines: exportingProduct.withGpuBackend ? ["TRACING_WITH_GPU_BACKEND"] : []
    }
}
