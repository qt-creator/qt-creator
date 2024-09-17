add_qtc_test(tst_connection_view
  EXCLUDE_FROM_PRECHECK # crashes on COIN since switching to Qt 6.8
  DEPENDS QmlJS Utils QmlDesigner
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${PROJECT_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES tst_connectioneditor.cpp
)
