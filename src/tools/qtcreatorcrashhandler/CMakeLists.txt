add_qtc_executable(qtcreator_crash_handler
  CONDITION UNIX AND NOT APPLE AND (CMAKE_BUILD_TYPE STREQUAL "Debug")
  DEPENDS app_version Utils Qt::Widgets
  SOURCES
    backtracecollector.cpp backtracecollector.h
    crashhandler.cpp crashhandler.h
    crashhandlerdialog.cpp crashhandlerdialog.h
    main.cpp
    utils.cpp utils.h
)
