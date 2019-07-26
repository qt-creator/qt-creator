add_qtc_test(tst_qml_check
  DEPENDS qmljs
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${PROJECT_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES tst_check.cpp
)
