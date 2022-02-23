set(StudioWelcomeDir "${PROJECT_SOURCE_DIR}/src/plugins/studiowelcome")

set(WITH_TESTS ON)

find_package(Googletest MODULE)

add_qtc_test(tst_qml_wizard
  DEPENDS Core Utils StudioWelcome ProjectExplorer QmlDesigner Googletest
  DEFINES
    QT_CREATOR
    QMLDESIGNER_TEST
    IDE_PLUGIN_PATH="${PROJECT_BINARY_DIR}/${IDE_PLUGIN_PATH}"
    IDE_DATA_PATH="${PROJECT_BINARY_DIR}/${IDE_DATA_PATH}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  INCLUDES "${PROJECT_SOURCE_DIR}/src/plugins"
           "${StudioWelcomeDir}"
  SOURCES
    wizardfactories-test.cpp
    stylemodel-test.cpp
    recentpresets-test.cpp
    userpresets-test.cpp
    presetmodel-test.cpp
    test-utilities.h
    test-main.cpp
    "${StudioWelcomeDir}/wizardfactories.cpp"
    "${StudioWelcomeDir}/stylemodel.cpp"
    "${StudioWelcomeDir}/recentpresets.cpp"
    "${StudioWelcomeDir}/userpresets.cpp"
    "${StudioWelcomeDir}/presetmodel.cpp"
)

