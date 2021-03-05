add_qtc_plugin(StudioWelcome
  DEPENDS Qt5::QuickWidgets
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  DEFINES STUDIO_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/qml/"
  SOURCES
    studiowelcomeplugin.cpp studiowelcomeplugin.h
    examplecheckout.cpp examplecheckout.h
    studiowelcome_global.h
    studiowelcome.qrc
    "${PROJECT_SOURCE_DIR}/src/share/3rdparty/studiofonts/studiofonts.qrc"
  EXTRA_TRANSLATIONS
    qml
)

if (TARGET StudioWelcome)
  file(GLOB_RECURSE qmlfiles
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    qml/*
  )
  qtc_add_resources(StudioWelcome StudioWelcome_qml FILES ${qmlfiles})
endif()
