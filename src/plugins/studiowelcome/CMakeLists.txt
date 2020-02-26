set(qmlQrcFile "${CMAKE_CURRENT_BINARY_DIR}/StudioWelcome_qml.qrc")

qtc_glob_resources(
  QRC_FILE "${qmlQrcFile}"
  ROOT "${CMAKE_CURRENT_SOURCE_DIR}"
  GLOB "qml/*"
)

add_qtc_plugin(StudioWelcome
  DEPENDS Qt5::QuickWidgets
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  DEFINES STUDIO_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/qml/"
  SOURCES
    studiowelcomeplugin.cpp studiowelcomeplugin.h
    studiowelcome_global.h
    studiowelcome.qrc
    ${qmlQrcFile}
    "${PROJECT_SOURCE_DIR}/src/share/3rdparty/studiofonts/studiofonts.qrc"
  EXTRA_TRANSLATIONS
    qml
)
