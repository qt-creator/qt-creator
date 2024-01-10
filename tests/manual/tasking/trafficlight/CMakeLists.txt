add_qtc_test(tst_tasking_trafficlight
  MANUALTEST
  DEPENDS Tasking Qt::Concurrent Qt::Network Qt::Widgets
  SOURCES
    glueinterface.h
    main.cpp
    recipe.cpp
    recipe.h
    trafficlight.cpp
    trafficlight.h
    trafficlight.qrc
)
