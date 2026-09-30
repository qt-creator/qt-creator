add_qtc_test(tst_qtprofiler_qmlrecordingtimeline
  DEPENDS Qt::Core
  INCLUDES ../../../../src/plugins/profiler
  SOURCES
    tst_qmlrecordingtimeline.cpp
    ../../../../src/plugins/profiler/qmlrecordingtimeline.h
)
