add_qtc_plugin(StudioWelcome
  CONDITION TARGET Qt::QuickWidgets AND TARGET QmlDesigner
  DEPENDS Qt::QuickWidgets Qt::QmlPrivate
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport QmlDesigner
  DEFINES STUDIO_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/qml/"
  SOURCES
    studiowelcomeplugin.cpp studiowelcomeplugin.h
    newprojectdialogimageprovider.cpp newprojectdialogimageprovider.h
    presetmodel.cpp presetmodel.h
    examplecheckout.cpp examplecheckout.h
    studiowelcome_global.h
    studiowelcometr.h
    qdsnewdialog.cpp qdsnewdialog.h
    wizardfactories.cpp wizardfactories.h
    createproject.cpp createproject.h
    wizardhandler.cpp wizardhandler.h
    userpresets.cpp userpresets.h
    screensizemodel.h
    algorithm.h
    stylemodel.h stylemodel.cpp
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

extend_qtc_plugin(StudioWelcome
  CONDITION BUILD_WITH_CRASHPAD
  DEFINES ENABLE_CRASHPAD
)
