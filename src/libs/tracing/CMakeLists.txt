add_qtc_library(Tracing
  DEPENDS Utils
  CONDITION TARGET Qt::CanvasPainter
  PUBLIC_DEPENDS Qt::Widgets Qt::CanvasPainter
  SOURCES
    flamegraphwidget.cpp flamegraphwidget.h
    safecastable.h
    timelinecoordinates.h
    timelineformattime.cpp timelineformattime.h
    trackpainter.cpp trackpainter.h
    trackpainterbase.cpp trackpainterbase.h
    trackpainterraster.cpp trackpainterraster.h
    tracklabels.cpp tracklabels.h
    timeruler.cpp timeruler.h
    rangedetailswidget.cpp rangedetailswidget.h
    selectionrangedetailswidget.cpp selectionrangedetailswidget.h
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

if(TARGET Tracing)
  qt_add_resources(Tracing "images"
    PREFIX "/tracing"
    BASE "."
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
