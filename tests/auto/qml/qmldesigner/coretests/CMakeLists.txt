# TODO: fix compiler errors
return()

add_qtc_test(tst_qml_testcore
  DEPENDS qmljs Utils QmlEditorWidgets CPlusPlus Core QmlJSEditor
  INCLUDES
    "${CMAKE_SOURCE_DIR}/src/plugins/qmldesigner/designercore/include"
    "${CMAKE_SOURCE_DIR}/src/plugins/qmldesigner/designercore"
    "${CMAKE_SOURCE_DIR}/share/qtcreator/qml/qmlpuppet/interfaces"
    "${CMAKE_SOURCE_DIR}/share/qtcreator/qml/qmlpuppet/types"
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${CMAKE_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES
    ../data/testfiles.qrc
    ../testview.cpp ../testview.h
    testrewriterview.cpp testrewriterview.h
    tst_testcore.cpp tst_testcore.h
)
