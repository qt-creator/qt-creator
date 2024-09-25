# qmldesigner/designercore/listmodeleditor
extend_qtc_test(unittest
  SOURCES
    listmodeleditor-test.cpp
)
extend_qtc_test(unittest
  SOURCES_PREFIX
    "${QML_DESIGNER_DIRECTORY}"
  SOURCES
    components/listmodeleditor/listmodeleditormodel.cpp components/listmodeleditor/listmodeleditormodel.h
)

extend_qtc_test(unittest
  SOURCES
    ${QT_CREATOR_LIBS_DIRECTORY}/qmlpuppetcommunication/interfaces/commondefines.h
    ${QML_DESIGNER_DIRECTORY}/components/listmodeleditor/listmodeleditormodel.cpp
    ${QML_DESIGNER_DIRECTORY}/components/listmodeleditor/listmodeleditormodel.h
)
