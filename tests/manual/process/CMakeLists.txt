add_qtc_test(tst_manual_qtcprocess
  MANUALTEST
  DEPENDS Utils
  SOURCES
    main.cpp
    mainwindow.cpp mainwindow.h
)

set_target_properties(tst_manual_qtcprocess PROPERTIES
  OUTPUT_NAME "process"
)
