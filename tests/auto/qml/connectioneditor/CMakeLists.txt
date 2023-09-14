add_qtc_test(tst_connection_view
  DEPENDS QmlJS Utils QmlDesigner
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${PROJECT_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES tst_connectioneditor.cpp
)
