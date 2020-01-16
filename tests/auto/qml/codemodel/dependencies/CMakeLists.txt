add_qtc_test(tst_qml_dependencies
  DEPENDS QmlJS QmlJSTools ExtensionSystem Utils
  INCLUDES "${PROJECT_SOURCE_DIR}/src/plugins"
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${PROJECT_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES tst_dependencies.cpp
)
