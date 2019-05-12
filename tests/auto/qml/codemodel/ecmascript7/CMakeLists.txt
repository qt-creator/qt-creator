add_qtc_test(tst_qml_ecmascript7
  DEPENDS qmljs QmlJSTools ExtensionSystem Utils
  INCLUDES "${CMAKE_SOURCE_DIR}/src/plugins"
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${CMAKE_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES tst_ecmascript7.cpp
)
