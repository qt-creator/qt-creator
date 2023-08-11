set(TRACING_CPP_SOURCES
  flamegraph.cpp flamegraph.h
  flamegraphattached.h
  safecastable.h
  timelineabstractrenderer.cpp timelineabstractrenderer.h timelineabstractrenderer_p.h
  timelineformattime.cpp timelineformattime.h
  timelineitemsrenderpass.cpp timelineitemsrenderpass.h
  timelinemodel.cpp timelinemodel.h timelinemodel_p.h
  timelinemodelaggregator.cpp timelinemodelaggregator.h
  timelinenotesmodel.cpp timelinenotesmodel.h timelinenotesmodel_p.h
  timelinenotesrenderpass.cpp timelinenotesrenderpass.h
  timelineoverviewrenderer.cpp timelineoverviewrenderer.h timelineoverviewrenderer_p.h
  timelinerenderer.cpp timelinerenderer.h timelinerenderer_p.h
  timelinerenderpass.cpp timelinerenderpass.h
  timelinerenderstate.cpp timelinerenderstate.h timelinerenderstate_p.h
  timelineselectionrenderpass.cpp timelineselectionrenderpass.h
  timelinetheme.cpp timelinetheme.h
  timelinetracefile.cpp timelinetracefile.h
  timelinetracemanager.cpp timelinetracemanager.h
  timelinezoomcontrol.cpp timelinezoomcontrol.h
  traceevent.h
  traceeventtype.h
  tracestashfile.h
  tracing_global.h
  tracingtr.h
)

find_package(Qt6 COMPONENTS ShaderTools QUIET)

add_qtc_library(Tracing
  CONDITION TARGET Qt6::ShaderTools AND TARGET Qt::Quick
  FEATURE_INFO
  DEPENDS Utils Qt::Quick
  PUBLIC_DEPENDS Qt::Widgets Qt::Qml
)

if (NOT TARGET Tracing)
  return()
endif()

set(TRACING_QML_FILES
  qml/ButtonsBar.qml
  qml/CategoryLabel.qml
  qml/Detail.qml
  qml/FlameGraphDelegate.qml
  qml/FlameGraphView.qml
  qml/ImageToolButton.qml
  qml/MainView.qml
  qml/Overview.qml
  qml/RangeDetails.qml
  qml/RangeMover.qml
  qml/RowLabel.qml
  qml/SelectionRange.qml
  qml/SelectionRangeDetails.qml
  qml/TimeDisplay.qml
  qml/TimelineContent.qml
  qml/TimelineLabels.qml
  qml/TimelineRulers.qml
  qml/TimelineText.qml
  qml/TimeMarks.qml
)

set(TRACING_QML_RESOURCES
  qml/ico_edit.png
  qml/ico_edit@2x.png
  qml/ico_rangeselected.png
  qml/ico_rangeselected@2x.png
  qml/ico_rangeselection.png
  qml/ico_rangeselection@2x.png
  qml/ico_selectionmode.png
  qml/ico_selectionmode@2x.png
  qml/range_handle.png
  qml/range_handle@2.png
)

foreach(file IN LISTS TRACING_QML_FILES TRACING_QML_RESOURCES)
  get_filename_component(fileName "${file}" NAME)
  set_source_files_properties("${file}" PROPERTIES QT_RESOURCE_ALIAS "${fileName}")
endforeach()

qt_add_shaders(Tracing "res_tracingshaders"
  BATCHABLE
  PREFIX
    "/qt/qml/QtCreator/Tracing"
  BASE
    "qml"
  FILES
    qml/notes_qt6.vert
    qml/notes_qt6.frag
    qml/timelineitems_qt6.vert
    qml/timelineitems_qt6.frag
)

qt_add_qml_module(Tracing
  URI "QtCreator.Tracing"
  VERSION "1.0"
  NO_PLUGIN
  RESOURCE_PREFIX "/qt/qml"
  DEPENDENCIES
    QtQuick
  QML_FILES
    ${TRACING_QML_FILES}
  RESOURCES
    ${TRACING_QML_RESOURCES}
  SOURCES
    ${TRACING_CPP_SOURCES}
)
