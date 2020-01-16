# TODO: fix compiler errors
return()

add_qtc_test(tst_qml_testcore
  DEPENDS QmlJS Utils QmlEditorWidgets CPlusPlus Core QmlJSEditor
  INCLUDES
    "${PROJECT_SOURCE_DIR}/src/plugins/qmldesigner/designercore/include"
    "${PROJECT_SOURCE_DIR}/src/plugins/qmldesigner/designercore"
    "${PROJECT_SOURCE_DIR}/share/qtcreator/qml/qmlpuppet/interfaces"
    "${PROJECT_SOURCE_DIR}/share/qtcreator/qml/qmlpuppet/types"
  DEFINES
    QT_CREATOR
    QTCREATORDIR="${PROJECT_SOURCE_DIR}"
    TESTSRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES
    ../data/testfiles.qrc
    ../testview.cpp ../testview.h
    testrewriterview.cpp testrewriterview.h
    tst_testcore.cpp tst_testcore.h
)
