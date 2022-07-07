add_qtc_test(tst_qml_importscheck
  EXCLUDE_FROM_PRECHECK
  DEPENDS QmlJS QmlJSTools Utils CPlusPlus
  INCLUDES "${PROJECT_SOURCE_DIR}/src/plugins"
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${PROJECT_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES tst_importscheck.cpp
)
