# qmldesigner/designercore/listmodeleditor
extend_qtc_test(unittest
  SOURCES
    listmodeleditor-test.cpp
)
extend_qtc_test(unittest
  SOURCES_PREFIX
    "${QmlDesignerDir}"
  SOURCES
    components/listmodeleditor/listmodeleditormodel.cpp components/listmodeleditor/listmodeleditormodel.h
)

extend_qtc_test(unittest
  SOURCES
    ${QtCreatorLibsDir}/qmlpuppetcommunication/interfaces/commondefines.h
    ${QmlDesignerDir}/components/listmodeleditor/listmodeleditormodel.cpp
    ${QmlDesignerDir}/components/listmodeleditor/listmodeleditormodel.h
)
