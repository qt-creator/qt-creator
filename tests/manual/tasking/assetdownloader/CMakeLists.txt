add_qtc_test(tst_tasking_assetdownloader
  MANUALTEST
  DEPENDS Tasking Qt::Concurrent Qt::Network Qt::Widgets Qt::GuiPrivate Qt::CorePrivate
  SOURCES
    assetdownloader.cpp
    assetdownloader.h
    main.cpp
)
