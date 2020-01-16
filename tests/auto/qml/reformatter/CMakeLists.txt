add_qtc_test(tst_qml_reformatter
  DEPENDS QmlJS
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${PROJECT_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES tst_reformatter.cpp
)
