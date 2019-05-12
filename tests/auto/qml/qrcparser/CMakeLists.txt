add_qtc_test(tst_qml_qrcparser
  DEPENDS qmljs Utils
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${CMAKE_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES tst_qrcparser.cpp
)
