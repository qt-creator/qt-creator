# qmldesigner/designercore/listmodeleditor
extend_qtc_test(unittest
  SOURCES
    listmodeleditor-test.cpp
)
extend_qtc_test(unittest
  INCLUDES
    "${QmlDesignerDir}"
    "${QmlDesignerDir}/designercore"
    "${QmlDesignerDir}/designercore/include"
    "${QmlDesignerDir}/designercore/imagecache"
    "${QmlDesignerDir}/../../../src/libs/qmlpuppetcommunication/interfaces"
    "${QmlDesignerDir}/../../../src/libs/qmlpuppetcommunication/types"
  DEFINES
    QMLDESIGNERCORE_STATIC_LIBRARY QMLDESIGNER_STATIC_LIBRARY
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
