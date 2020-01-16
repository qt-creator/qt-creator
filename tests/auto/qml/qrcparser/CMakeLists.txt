add_qtc_test(tst_qml_qrcparser
  DEPENDS QmlJS Utils
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${PROJECT_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES tst_qrcparser.cpp
)
