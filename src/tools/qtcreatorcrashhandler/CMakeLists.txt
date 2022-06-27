add_qtc_executable(qtcreator_crash_handler
  CONDITION UNIX AND NOT APPLE AND (CMAKE_BUILD_TYPE STREQUAL "Debug")
  DEPENDS app_version Utils Qt5::Widgets
  SOURCES
    backtracecollector.cpp backtracecollector.h
    crashhandler.cpp crashhandler.h
    crashhandlerdialog.cpp crashhandlerdialog.h crashhandlerdialog.ui
    main.cpp
    utils.cpp utils.h
)
