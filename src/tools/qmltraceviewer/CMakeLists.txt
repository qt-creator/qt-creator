add_qtc_executable(qmltraceviewer
  CONDITION TARGET QmlProfiler
  DEPENDS app_version QmlProfiler
  SOURCES
    qmltraceviewerinit.cpp qmltraceviewerinit.h
    qmltraceviewermain.cpp
    qmltraceviewersettings.cpp qmltraceviewersettings.h
    qmltraceviewerwindow.h qmltraceviewerwindow.cpp
    schema/api.h
)
