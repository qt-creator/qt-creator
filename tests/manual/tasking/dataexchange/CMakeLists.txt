add_qtc_test(tst_tasking_dataexchange
  MANUALTEST
  DEPENDS Tasking Qt::Concurrent Qt::Network Qt::Widgets
  SOURCES
    main.cpp
    recipe.cpp
    recipe.h
    viewer.cpp
    viewer.h
)
