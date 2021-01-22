set(SRCDIR "${PROJECT_SOURCE_DIR}/share/qtcreator/qml/qmlpuppet")

set(DESTDIR "${IDE_LIBEXEC_PATH}")
if (APPLE)
  string(APPEND DESTDIR "/qmldesigner")
endif()

add_qtc_library(qml2puppet_static
  STATIC
  DEPENDS
  Qt5::CorePrivate Qt5::Widgets
  PUBLIC_INCLUDES "${SRCDIR}/interfaces"
  SOURCES_PREFIX "${SRCDIR}/interfaces"
  SOURCES
    commondefines.h
    nodeinstanceclientinterface.h
    nodeinstanceglobal.h
    nodeinstanceserverinterface.cpp nodeinstanceserverinterface.h
)

extend_qtc_library(qml2puppet_static
  PUBLIC_INCLUDES "${SRCDIR}/types"
  SOURCES "${SRCDIR}/types/enumeration.h"
)

extend_qtc_library(qml2puppet_static
  PUBLIC_INCLUDES "${SRCDIR}/container"
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

extend_qtc_library(qml2puppet_static
  CONDITION UNIX
  SOURCES_PREFIX "${SRCDIR}/container"
  SOURCES
    sharedmemory_unix.cpp
)

extend_qtc_library(qml2puppet_static
  CONDITION NOT UNIX
  SOURCES_PREFIX "${SRCDIR}/container"
  SOURCES
    sharedmemory_qt.cpp
)

extend_qtc_library(qml2puppet_static
  PUBLIC_INCLUDES "${SRCDIR}/commands"
  SOURCES_PREFIX "${SRCDIR}/commands"
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
    changepreviewimagesizecommand.cpp changepreviewimagesizecommand.h
    changelanguagecommand.cpp changelanguagecommand.h
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
    update3dviewstatecommand.cpp update3dviewstatecommand.h
    puppettocreatorcommand.cpp puppettocreatorcommand.h
    inputeventcommand.cpp inputeventcommand.h
    view3dactioncommand.cpp view3dactioncommand.h
    requestmodelnodepreviewimagecommand.cpp requestmodelnodepreviewimagecommand.h
    valueschangedcommand.cpp
    captureddatacommand.h
    scenecreatedcommand.h
)

add_qtc_executable(qml2puppet
  DEPENDS
    Qt5::CorePrivate Qt5::Widgets Qt5::QmlPrivate
    Qt5::QuickPrivate Qt5::Network Qt5::GuiPrivate
    qml2puppet_static
  SOURCES
    "${SRCDIR}/qml2puppet/qml2puppetmain.cpp"
    "${SRCDIR}/qmlpuppet.qrc"
  DESTINATION ${DESTDIR}
)

extend_qtc_executable(qml2puppet
  CONDITION UNIX AND (NOT APPLE)
  DEPENDS rt
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX "${SRCDIR}/instances"
  SOURCES
    nodeinstanceclientproxy.cpp nodeinstanceclientproxy.h
)

find_package(Qt5 5.15.0 COMPONENTS Quick3D QUIET)
extend_qtc_executable(qml2puppet
  CONDITION TARGET Qt5::Quick3D
  FEATURE_INFO "Qt Quick 3D support"
  DEPENDS Qt5::Quick3D Qt5::Quick3DPrivate
  DEFINES QUICK3D_MODULE

  SOURCES_PREFIX "${SRCDIR}/qml2puppet/editor3d"
  SOURCES
    generalhelper.cpp generalhelper.h
    mousearea3d.cpp mousearea3d.h
    camerageometry.cpp camerageometry.h
    lightgeometry.cpp lightgeometry.h
    gridgeometry.cpp gridgeometry.h
    selectionboxgeometry.cpp selectionboxgeometry.h
    linegeometry.cpp linegeometry.h
    icongizmoimageprovider.cpp icongizmoimageprovider.h
)

find_package(Qt5 5.15.0 COMPONENTS Quick3DAssetImport QUIET)
extend_qtc_executable(qml2puppet
  CONDITION TARGET Qt5::Quick3DAssetImport
  FEATURE_INFO "Qt Quick 3D asset import"
  DEPENDS Qt5::Quick3DAssetImportPrivate
  DEFINES IMPORT_QUICK3D_ASSETS
)

extend_qtc_executable(qml2puppet
  CONDITION Qt5_VERSION VERSION_GREATER_EQUAL 6.0.0

  SOURCES_PREFIX "${SRCDIR}/qml2puppet/editor3d/qt5compat"
  SOURCES
    qquick3darealight.cpp qquick3darealight_p.h
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX "${SRCDIR}/qml2puppet/iconrenderer"
  SOURCES
    iconrenderer.cpp iconrenderer.h
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX "${SRCDIR}/qml2puppet/import3d"
  SOURCES
    import3d.cpp import3d.h
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
    quick3dtexturenodeinstance.cpp quick3dtexturenodeinstance.h
    quickitemnodeinstance.cpp quickitemnodeinstance.h
    servernodeinstance.cpp servernodeinstance.h
    qt5capturepreviewnodeinstanceserver.cpp qt5capturepreviewnodeinstanceserver.h
    nodeinstanceserverdispatcher.cpp nodeinstanceserverdispatcher.h
    capturenodeinstanceserverdispatcher.cpp capturenodeinstanceserverdispatcher.h
    qt5captureimagenodeinstanceserver.cpp qt5captureimagenodeinstanceserver.h
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

if(DEFINED MULTILANGUAGE_SUPPORT_SUBDIRECTORY)
  add_subdirectory(${MULTILANGUAGE_SUPPORT_SUBDIRECTORY} multilanguagesupport_static_build)
endif()

extend_qtc_executable(qml2puppet
    CONDITION TARGET QtCreator::multilanguage-support
    DEPENDS QtCreator::multilanguage-support
    FEATURE_INFO "multilanguage-support in qml2puppet"
)

# Crashpad
# only windows requires separate crashpad client per process until client->SetHandlerIPCPipe()
# is implemented (check the TODO inside startCrashpad())
extend_qtc_executable(qml2puppet
  CONDITION BUILD_WITH_CRASHPAD AND WIN32
  DEFINES
    CRASHPAD_BACKEND_URL="${CRASHPAD_BACKEND_URL}"
    ENABLE_CRASHPAD
  DEPENDS Crashpad::Crashpad
)
