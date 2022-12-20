add_qtc_library(QmlPuppetCommunication STATIC
    PROPERTIES AUTOUIC OFF
    DEPENDS Qt::Core Qt::CorePrivate Qt::Gui
)

extend_qtc_library(QmlPuppetCommunication
  CONDITION TARGET Nanotrace
  PUBLIC_DEPENDS Nanotrace
)

extend_qtc_library(QmlPuppetCommunication
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/types
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/types
  SOURCES
    enumeration.h
)

extend_qtc_library(QmlPuppetCommunication
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/interfaces
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/interfaces
  SOURCES
    commondefines.h
    nodeinstanceclientinterface.h
    nodeinstanceserverinterface.cpp nodeinstanceserverinterface.h
    nodeinstanceglobal.h
)

extend_qtc_library(QmlPuppetCommunication
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/container
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/container
  SOURCES
    addimportcontainer.cpp addimportcontainer.h
    idcontainer.cpp idcontainer.h
    imagecontainer.cpp imagecontainer.h
    informationcontainer.cpp informationcontainer.h
    instancecontainer.cpp instancecontainer.h
    mockuptypecontainer.cpp mockuptypecontainer.h
    propertyabstractcontainer.cpp propertyabstractcontainer.h
    propertybindingcontainer.cpp propertybindingcontainer.h
    propertyvaluecontainer.cpp propertyvaluecontainer.h
    reparentcontainer.cpp reparentcontainer.h
    sharedmemory.h
)

extend_qtc_library(QmlPuppetCommunication
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/container
  CONDITION UNIX
  SOURCES
    sharedmemory_unix.cpp
)

extend_qtc_library(QmlPuppetCommunication
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/container
  CONDITION NOT UNIX
  SOURCES
    sharedmemory_qt.cpp
)

extend_qtc_library(QmlPuppetCommunication
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/commands
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/commands
  SOURCES
    captureddatacommand.h
    changeauxiliarycommand.cpp changeauxiliarycommand.h
    changebindingscommand.cpp changebindingscommand.h
    changefileurlcommand.cpp changefileurlcommand.h
    changeidscommand.cpp  changeidscommand.h
    changelanguagecommand.cpp changelanguagecommand.h
    changenodesourcecommand.cpp changenodesourcecommand.h
    changepreviewimagesizecommand.cpp changepreviewimagesizecommand.h
    changeselectioncommand.cpp changeselectioncommand.h
    changestatecommand.cpp changestatecommand.h
    changevaluescommand.cpp changevaluescommand.h
    childrenchangedcommand.cpp childrenchangedcommand.h
    clearscenecommand.cpp clearscenecommand.h
    completecomponentcommand.cpp completecomponentcommand.h
    componentcompletedcommand.cpp componentcompletedcommand.h
    createinstancescommand.cpp createinstancescommand.h
    createscenecommand.cpp createscenecommand.h
    debugoutputcommand.cpp debugoutputcommand.h
    endpuppetcommand.cpp endpuppetcommand.h
    informationchangedcommand.cpp informationchangedcommand.h
    inputeventcommand.cpp inputeventcommand.h
    nanotracecommand.cpp nanotracecommand.h
    pixmapchangedcommand.cpp pixmapchangedcommand.h
    puppetalivecommand.cpp puppetalivecommand.h
    puppettocreatorcommand.cpp puppettocreatorcommand.h
    removeinstancescommand.cpp removeinstancescommand.h
    removepropertiescommand.cpp removepropertiescommand.h
    removesharedmemorycommand.cpp removesharedmemorycommand.h
    reparentinstancescommand.cpp reparentinstancescommand.h
    requestmodelnodepreviewimagecommand.cpp requestmodelnodepreviewimagecommand.h
    scenecreatedcommand.h
    statepreviewimagechangedcommand.cpp statepreviewimagechangedcommand.h
    synchronizecommand.h
    tokencommand.cpp tokencommand.h
    update3dviewstatecommand.cpp update3dviewstatecommand.h
    valueschangedcommand.cpp valueschangedcommand.h
    view3dactioncommand.cpp view3dactioncommand.h
)
