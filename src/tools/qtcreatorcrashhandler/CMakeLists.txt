if (LINUX) # Debug build only!
  add_qtc_executable(qtcreatorcrashhandler
    DEPENDS Utils Qt5::Widgets
    SOURCES
      backtracecollector.cpp backtracecollector.h
      crashhandler.cpp crashhandler.h
      crashhandlerdialog.cpp crashhandlerdialog.h crashhandlerdialog.ui
      main.cpp
      utils.cpp utils.h
  )
endif()
