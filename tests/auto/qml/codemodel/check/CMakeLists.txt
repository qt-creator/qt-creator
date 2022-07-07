add_qtc_test(tst_qml_check
  EXCLUDE_FROM_PRECHECK
  DEPENDS QmlJS QmlJSTools ExtensionSystem Utils
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${PROJECT_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES tst_check.cpp
)
