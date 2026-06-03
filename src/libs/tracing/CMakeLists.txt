set(TRACING_CPP_SOURCES
  flamegraphwidget.cpp flamegraphwidget.h
  safecastable.h
  timelinecoordinates.h
  timelineformattime.cpp timelineformattime.h
  trackpainter.cpp trackpainter.h
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

set(TRACING_ICON_RESOURCES
  qml/ico_edit.png
  qml/ico_edit@2x.png
  qml/ico_rangeselected.png
  qml/ico_rangeselected@2x.png
  qml/ico_rangeselection.png
  qml/ico_rangeselection@2x.png
  qml/ico_selectionmode.png
  qml/ico_selectionmode@2x.png
)

foreach(file IN LISTS TRACING_ICON_RESOURCES)
  get_filename_component(fileName "${file}" NAME)
  set_source_files_properties("${file}" PROPERTIES QT_RESOURCE_ALIAS "${fileName}")
endforeach()

add_qtc_library(Tracing
  DEPENDS Utils
  PUBLIC_DEPENDS Qt::Widgets
  SOURCES
    ${TRACING_CPP_SOURCES}
)

qt_add_resources(Tracing "tracing_icons"
  PREFIX "/qt/qml/QtCreator/Tracing"
  BASE "qml"
  FILES
    ${TRACING_ICON_RESOURCES}
)

extend_qtc_library(Tracing
  INCLUDES
    ../utils/theme
)
