add_qtc_library(Tracing
  DEPENDS Utils
  PUBLIC_DEPENDS Qt::Widgets
  SOURCES
    flamegraphwidget.cpp flamegraphwidget.h
    safecastable.h
    timelinecoordinates.h
    timelineformatdata.cpp timelineformatdata.h
    trackpainterbase.cpp trackpainterbase.h
    trackpainterraster.cpp trackpainterraster.h
    tracklabels.cpp tracklabels.h
    timeruler.cpp timeruler.h
    timelinedetailswidget.cpp timelinedetailswidget.h
    pausedrangesoverlay.cpp pausedrangesoverlay.h
    selectionrangeoverlay.cpp selectionrangeoverlay.h
    timelinescrollsync.cpp timelinescrollsync.h
    timelinecontentwidget.cpp timelinecontentwidget.h
    timelineoverviewwidget.cpp timelineoverviewwidget.h
    timelinemodel.cpp timelinemodel.h timelinemodel_p.h
    timelinemodelaggregator.cpp timelinemodelaggregator.h
    timelinenotesmodel.cpp timelinenotesmodel.h
    timelinetracefile.cpp timelinetracefile.h
    timelinetracemanager.cpp timelinetracemanager.h
    timelinewidget.cpp timelinewidget.h
    timelinezoomcontrol.cpp timelinezoomcontrol.h
    traceevent.h
    traceeventtype.h
    tracestashfile.h
    tracing_global.h
    tracingtr.h
)

# The GPU track backend draws through CanvasPainter, and is only ever chosen on
# macOS and Windows (see resolvedTrackBackend()). Without CanvasPainter only the
# software backend is built. On WebAssembly the GPU backend would be dead code
# that still pulls CanvasPainter, and Qt Quick behind it, into the binary, so
# leave it out there as well.
extend_qtc_library(Tracing
  CONDITION TARGET Qt::CanvasPainter AND NOT EMSCRIPTEN
  PUBLIC_DEPENDS Qt::CanvasPainter
  PUBLIC_DEFINES TRACING_WITH_GPU_BACKEND
  SOURCES trackpaintergpu.cpp trackpaintergpu.h
)

if(TARGET Tracing)
  qt_add_resources(Tracing "images"
    PREFIX "/tracing"
    FILES
      images/edit.png
      images/edit@2x.png
      images/rangeselected.png
      images/rangeselected@2x.png
      images/rangeselection.png
      images/rangeselection@2x.png
      images/selectionmode.png
      images/selectionmode@2x.png
  )
endif()
