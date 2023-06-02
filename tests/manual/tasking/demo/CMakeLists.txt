add_qtc_test(tst_tasking_demo
  MANUALTEST
  DEPENDS Tasking Qt::Widgets Qt::Network
  SOURCES
    demo.qrc
    main.cpp
    progressindicator.h
    progressindicator.cpp
    taskwidget.h
    taskwidget.cpp
)
