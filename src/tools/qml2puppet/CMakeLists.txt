set(SRCDIR "${PROJECT_SOURCE_DIR}/share/qtcreator/qml/qmlpuppet")

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

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX "${SRCDIR}/commands"
  INCLUDES "${PROJECT_SOURCE_DIR}/src/libs"
  SOURCES
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
    changeselectioncommand.cpp changeselectioncommand.h
    drop3dlibraryitemcommand.cpp drop3dlibraryitemcommand.h
    update3dviewstatecommand.cpp update3dviewstatecommand.h
    enable3dviewcommand.cpp enable3dviewcommand.h
    view3dclosedcommand.cpp view3dclosedcommand.h
    puppettocreatorcommand.cpp puppettocreatorcommand.h
    valueschangedcommand.cpp
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX "${SRCDIR}/container"
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

extend_qtc_executable(qml2puppet
  CONDITION UNIX
  SOURCES_PREFIX "${SRCDIR}/container"
  SOURCES
    sharedmemory_unix.cpp
)
extend_qtc_executable(qml2puppet
  CONDITION UNIX AND (NOT APPLE)
  DEPENDS rt
)
extend_qtc_executable(qml2puppet
  CONDITION NOT UNIX
  SOURCES_PREFIX "${SRCDIR}/container"
  SOURCES
    sharedmemory_qt.cpp
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX "${SRCDIR}/instances"
  SOURCES
    nodeinstanceclientproxy.cpp nodeinstanceclientproxy.h
)

find_package(Qt5 COMPONENTS Quick3D QUIET)
extend_qtc_executable(qml2puppet
  CONDITION TARGET Qt5::Quick3
  FEATURE_INFO "Qt Quick 3D"
  DEPENDS Qt5::Quick3D
  DEFINES QUICK3D_MODULE
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX "${SRCDIR}/interfaces"
  SOURCES
    commondefines.h
    nodeinstanceclientinterface.h
    nodeinstanceglobal.h
    nodeinstanceserverinterface.cpp nodeinstanceserverinterface.h
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX "${SRCDIR}/qml2puppet/editor3d"
  SOURCES
    generalhelper.cpp generalhelper.h
    mousearea3d.cpp mousearea3d.h
    camerageometry.cpp camerageometry.h
    gridgeometry.cpp gridgeometry.h
    selectionboxgeometry.cpp selectionboxgeometry.h
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX "${SRCDIR}/qml2puppet/instances"
  SOURCES
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
    quick3dnodeinstance.cpp quick3dnodeinstance.h
    quickitemnodeinstance.cpp quickitemnodeinstance.h
    servernodeinstance.cpp servernodeinstance.h
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX "${SRCDIR}/types"
  SOURCES
    enumeration.h
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX "${SRCDIR}/qmlprivategate"
  SOURCES
    designersupportdelegate.h
    qmlprivategate_56.cpp qmlprivategate.h
)
