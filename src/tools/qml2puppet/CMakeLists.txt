set(SRCDIR "${CMAKE_SOURCE_DIR}/share/qtcreator/qml/qmlpuppet")

set(DESTDIR "${IDE_LIBEXEC_PATH}")
if (APPLE)
  string(APPEND DESTDIR "/qmldesigner")
endif()

add_qtc_executable(qml2puppet
  DEPENDS
    Qt5::CorePrivate Qt5::Widgets Qt5::QmlPrivate
    Qt5::QuickPrivate Qt5::Network Qt5::GuiPrivate
  SOURCES
    "${SRCDIR}/qml2puppet/qml2puppetmain.cpp"
    "${SRCDIR}/qmlpuppet.qrc"
  DESTINATION ${DESTDIR}
)

function(extend_target name directory)
  foreach(source ${ARGN})
    list(APPEND source_list "${directory}/${source}")
  endforeach()
  target_sources(${name} PRIVATE ${source_list})
  target_include_directories(${name} PUBLIC ${directory})
endfunction()

extend_target(qml2puppet "${SRCDIR}/commands"
  changeauxiliarycommand.cpp changeauxiliarycommand.h
  changebindingscommand.cpp changebindingscommand.h
  changefileurlcommand.cpp changefileurlcommand.h
  changeidscommand.cpp changeidscommand.h
  changenodesourcecommand.cpp changenodesourcecommand.h
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
  pixmapchangedcommand.cpp pixmapchangedcommand.h
  puppetalivecommand.cpp puppetalivecommand.h
  removeinstancescommand.cpp removeinstancescommand.h
  removepropertiescommand.cpp removepropertiescommand.h
  removesharedmemorycommand.cpp removesharedmemorycommand.h
  reparentinstancescommand.cpp reparentinstancescommand.h
  statepreviewimagechangedcommand.cpp statepreviewimagechangedcommand.h
  synchronizecommand.cpp synchronizecommand.h
  tokencommand.cpp tokencommand.h
  valueschangedcommand.cpp
)

extend_target(qml2puppet "${SRCDIR}/container"
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

if (UNIX)
  extend_target(qml2puppet "${SRCDIR}/container" sharedmemory_unix.cpp)
  if (NOT APPLE)
    target_link_libraries(qml2puppet PRIVATE rt)
  endif()
else()
  extend_target(qml2puppet "${SRCDIR}/container" sharedmemory_qt.cpp)
endif()

extend_target(qml2puppet "${SRCDIR}/instances"
  nodeinstanceclientproxy.cpp nodeinstanceclientproxy.h
)

extend_target(qml2puppet "${SRCDIR}/interfaces"
  commondefines.h
  nodeinstanceclientinterface.h
  nodeinstanceglobal.h
  nodeinstanceserverinterface.cpp nodeinstanceserverinterface.h
)

extend_target(qml2puppet "${SRCDIR}/qml2puppet/instances"
  anchorchangesnodeinstance.cpp anchorchangesnodeinstance.h
  behaviornodeinstance.cpp behaviornodeinstance.h
  childrenchangeeventfilter.cpp childrenchangeeventfilter.h
  componentnodeinstance.cpp componentnodeinstance.h
  dummycontextobject.cpp dummycontextobject.h
  dummynodeinstance.cpp dummynodeinstance.h
  layoutnodeinstance.cpp layoutnodeinstance.h
  nodeinstanceserver.cpp nodeinstanceserver.h
  nodeinstancesignalspy.cpp nodeinstancesignalspy.h
  objectnodeinstance.cpp objectnodeinstance.h
  positionernodeinstance.cpp positionernodeinstance.h
  qmlpropertychangesnodeinstance.cpp qmlpropertychangesnodeinstance.h
  qmlstatenodeinstance.cpp qmlstatenodeinstance.h
  qmltransitionnodeinstance.cpp qmltransitionnodeinstance.h
  qt3dpresentationnodeinstance.cpp qt3dpresentationnodeinstance.h
  qt5informationnodeinstanceserver.cpp qt5informationnodeinstanceserver.h
  qt5nodeinstanceclientproxy.cpp qt5nodeinstanceclientproxy.h
  qt5nodeinstanceserver.cpp qt5nodeinstanceserver.h
  qt5previewnodeinstanceserver.cpp qt5previewnodeinstanceserver.h
  qt5rendernodeinstanceserver.cpp qt5rendernodeinstanceserver.h
  qt5testnodeinstanceserver.cpp qt5testnodeinstanceserver.h
  quickitemnodeinstance.cpp quickitemnodeinstance.h
  servernodeinstance.cpp servernodeinstance.h
)

extend_target(qml2puppet "${SRCDIR}/types"
  enumeration.cpp enumeration.h
)

extend_target(qml2puppet "${SRCDIR}/qmlprivategate"
  designersupportdelegate.h
  qmlprivategate_56.cpp qmlprivategate.h
)
