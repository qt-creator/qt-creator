add_qtc_test(tst_tasking_imagescaling
  MANUALTEST
  DEPENDS Tasking Qt::Concurrent Qt::Network Qt::Widgets
  SOURCES
    downloaddialog.cpp
    downloaddialog.h
    downloaddialog.ui
    imagescaling.cpp
    imagescaling.h
    main.cpp
)
