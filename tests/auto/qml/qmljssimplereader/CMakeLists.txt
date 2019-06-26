add_qtc_test(tst_qml_qmljssimplereader
  DEPENDS qmljs
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${PROJECT_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES tst_qmljssimplereader.cpp
)
