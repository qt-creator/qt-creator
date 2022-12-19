cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")


project(qml2puppet)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT QT_CREATOR_API_DEFINED)
  # standalone build
  set(DESTINATION DESTINATION .)
  include(QtCreatorIDEBranding)
  include(QtCreatorAPI)
endif()

find_package(Qt5
  COMPONENTS Concurrent Core Gui Network PrintSupport Qml Quick Sql Widgets Xml Core5Compat
  REQUIRED
)

if (NOT TARGET QmlPuppetCommunication)
include(../../libs/qmlpuppetcommunication/QmlPuppetCommunication.cmake)
endif()

add_qtc_executable(qml2puppet
  ${DESTINATION}
  DEPENDS
    Qt5::CorePrivate Qt5::Widgets Qt5::QmlPrivate
    Qt5::QuickPrivate Qt5::Network Qt5::GuiPrivate
    QmlPuppetCommunication
  SOURCES
    qml2puppet/qml2puppetmain.cpp
    qmlpuppet.qrc
  PROPERTIES
    OUTPUT_NAME qml2puppet-${IDE_VERSION}
)

extend_qtc_executable(qml2puppet
  CONDITION Qt5_VERSION VERSION_GREATER_EQUAL 6.0.0
  SOURCES
    editor3d_qt6.qrc
)

extend_qtc_executable(qml2puppet
  CONDITION Qt5_VERSION VERSION_LESS 6.0.0
  SOURCES
    editor3d_qt5.qrc
)

extend_qtc_executable(qml2puppet
  CONDITION UNIX AND (NOT APPLE)
  DEPENDS rt
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX instances
  SOURCES
    nodeinstanceclientproxy.cpp nodeinstanceclientproxy.h
)

find_package(Qt5 5.15.0 COMPONENTS Quick3D QUIET)
extend_qtc_executable(qml2puppet
  CONDITION TARGET Qt5::Quick3D
  FEATURE_INFO "Qt Quick 3D support"
  DEPENDS Qt5::Quick3D Qt5::Quick3DPrivate
  DEFINES QUICK3D_MODULE

  SOURCES_PREFIX qml2puppet/editor3d
  SOURCES
    generalhelper.cpp generalhelper.h
    mousearea3d.cpp mousearea3d.h
    geometrybase.cpp geometrybase.h
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

find_package(Qt5 5.15.0 COMPONENTS Quick3DParticles QUIET)
extend_qtc_executable(qml2puppet
  CONDITION TARGET Qt5::Quick3DParticles
  FEATURE_INFO "Qt Quick 3D particles"
  DEPENDS Qt5::Quick3DParticles Qt5::Quick3DParticlesPrivate
  DEFINES QUICK3D_PARTICLES_MODULE
)

# Quick3DAssetUtils optionally depends on QuickTimeline, so find also it to make the CI build work
find_package(Qt5 5.15.0 COMPONENTS Quick3DAssetUtils QuickTimeline QUIET)
extend_qtc_executable(qml2puppet
  CONDITION TARGET Qt5::Quick3DAssetUtils
  FEATURE_INFO "Qt Quick 3D asset utils"
  DEPENDS Qt5::Quick3DAssetUtilsPrivate
  DEFINES QUICK3D_ASSET_UTILS_MODULE
)

extend_qtc_executable(qml2puppet
  CONDITION Qt5_VERSION VERSION_GREATER_EQUAL 6.0.0
  SOURCES_PREFIX qml2puppet/editor3d/qt5compat
  SOURCES
    qquick3darealight.cpp qquick3darealight_p.h
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX qml2puppet/iconrenderer
  SOURCES
    iconrenderer.cpp iconrenderer.h
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX qml2puppet/import3d
  SOURCES
    import3d.cpp import3d.h
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX qml2puppet/instances
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
    quick3dmaterialnodeinstance.cpp quick3dmaterialnodeinstance.h
    quick3dnodeinstance.cpp quick3dnodeinstance.h
    quick3drenderablenodeinstance.cpp quick3drenderablenodeinstance.h
    quick3dtexturenodeinstance.cpp quick3dtexturenodeinstance.h
    quickitemnodeinstance.cpp quickitemnodeinstance.h
    servernodeinstance.cpp servernodeinstance.h
    qt5capturepreviewnodeinstanceserver.cpp qt5capturepreviewnodeinstanceserver.h
    nodeinstanceserverdispatcher.cpp nodeinstanceserverdispatcher.h
    capturenodeinstanceserverdispatcher.cpp capturenodeinstanceserverdispatcher.h
    qt5captureimagenodeinstanceserver.cpp qt5captureimagenodeinstanceserver.h
    viewconfig.cpp viewconfig.h
    animationdriver.cpp animationdriver.h
)

extend_qtc_executable(qml2puppet
  SOURCES_PREFIX qmlprivategate
  SOURCES
    qmlprivategate.cpp qmlprivategate.h
)

if(DEFINED MULTILANGUAGE_SUPPORT_SUBDIRECTORY AND Qt6_VERSION VERSION_GREATER_EQUAL 6.2.1)
  add_subdirectory(${MULTILANGUAGE_SUPPORT_SUBDIRECTORY} multilanguagesupport_static_build)
endif()

extend_qtc_executable(qml2puppet
    CONDITION TARGET QtCreator::multilanguage-support
    DEPENDS QtCreator::multilanguage-support
    FEATURE_INFO "multilanguage-support in qml2puppet"
)

extend_qtc_executable(qml2puppet
  PUBLIC_INCLUDES src/libs
)

extend_qtc_executable(qml2puppet
  CONDITION TARGET Nanotrace
  DEPENDS Nanotrace
)

if (QTC_STATIC_BUILD AND Qt5_VERSION VERSION_GREATER_EQUAL 6.0.0)
  qt6_import_qml_plugins(qml2puppet PATH_TO_SCAN ${SRCDIR})
endif()

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
