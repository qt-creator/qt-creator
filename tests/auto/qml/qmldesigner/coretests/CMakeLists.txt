add_qtc_test(tst_qml_testcore
  NEEDS_GUI
  EXCLUDE_FROM_PRECHECK
  CONDITION TARGET QmlProjectManager
  DEPENDS
    Sqlite
    QmlDesignerCore
    Qt::Widgets
    Qt::Qml
  DEFINES
    QT_CREATOR
    IDE_PLUGIN_PATH="${PROJECT_BINARY_DIR}/${IDE_PLUGIN_PATH}"
    IDE_DATA_PATH="${PROJECT_BINARY_DIR}/${IDE_DATA_PATH}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    QMLDESIGNERCORE_STATIC_LIBRARY QMLDESIGNERUTILS_STATIC_LIBRARY
  INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}/../../../../../share/qtcreator/qml/qmlpuppet/commands"
  SOURCES
    ../data/testfiles.qrc
    ../testview.cpp
    ../testview.h
    testrewriterview.cpp
    testrewriterview.h
    tst_testcore.cpp
    tst_testcore.h
    ../testconnectionmanager.cpp
    ../testconnectionmanager.h
)

