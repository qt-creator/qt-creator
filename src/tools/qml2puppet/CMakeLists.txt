cmake_minimum_required(VERSION 3.16)

if (NOT QT_CREATOR_API_DEFINED)
  # standalone build
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

  project(qml2puppet)

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  set(DESTINATION DESTINATION .)
  include(QtCreatorIDEBranding)
  include(QtCreatorAPI)
  qtc_handle_compiler_cache_support()

  find_package(Qt6
    COMPONENTS Concurrent Core Gui Network PrintSupport Qml Quick Sql Widgets Xml
    REQUIRED
  )
endif()

if (NOT TARGET QmlPuppetCommunication)
  include(../../libs/qmlpuppetcommunication/QmlPuppetCommunication.cmake)
endif()

add_qtc_executable(qml2puppet
  ${DESTINATION}
  CONDITION TARGET Qt::Quick
  DEPENDS
    Qt::CorePrivate Qt::Widgets Qt::QmlPrivate
    Qt::QuickPrivate Qt::Network Qt::GuiPrivate
    QmlPuppetCommunication
  INCLUDES
    ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    qml2puppet/qml2puppetmain.cpp
    qml2puppet/qmlbase.h
    qml2puppet/appmetadata.cpp qml2puppet/appmetadata.h
    qml2puppet/qmlpuppet.h qml2puppet/qmlpuppet.cpp qml2puppet/configcrashpad.h
    qmlpuppet.qrc
  PROPERTIES
    OUTPUT_NAME qml2puppet-${IDE_VERSION}
)

if (TARGET qml2puppet)
  execute_process(
      COMMAND git describe --tags --always --dirty=+
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      RESULT_VARIABLE GIT_SHA_RESULT
      OUTPUT_VARIABLE GIT_SHA
      ERROR_VARIABLE GIT_SHA_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  #if we are not a git repository use the .tag file
  if(NOT GIT_SHA)
      file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/../../../.tag GIT_SHA LIMIT_COUNT 1)
  endif()

  set(IDE_REVISION_STR ${GIT_SHA})

  configure_file(../../app/app_version.h.cmakein app/app_version.h ESCAPE_QUOTES)
endif()

extend_qtc_executable(qml2puppet
  CONDITION Qt6_VERSION
  SOURCES
    editor3d_qt6.qrc
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

find_package(Qt6 COMPONENTS Quick3D QUIET)
extend_qtc_executable(qml2puppet
  CONDITION TARGET Qt::Quick3D
  FEATURE_INFO "Qt Quick 3D support"
  DEPENDS Qt::Quick3D Qt::Quick3DPrivate
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

find_package(Qt6 COMPONENTS Quick3DAssetImport QUIET)
extend_qtc_executable(qml2puppet
  CONDITION TARGET Qt::Quick3DAssetImport
  FEATURE_INFO "Qt Quick 3D asset import"
  DEPENDS Qt::Quick3DAssetImportPrivate
  DEFINES IMPORT_QUICK3D_ASSETS
)

find_package(Qt6 COMPONENTS Quick3DParticles QUIET)
extend_qtc_executable(qml2puppet
  CONDITION TARGET Qt::Quick3DParticles
  FEATURE_INFO "Qt Quick 3D particles"
  DEPENDS Qt::Quick3DParticles Qt::Quick3DParticlesPrivate
  DEFINES QUICK3D_PARTICLES_MODULE
)

# Quick3DAssetUtils optionally depends on QuickTimeline, so find also it to make the CI build work
find_package(Qt6 COMPONENTS Quick3DAssetUtils QuickTimeline QUIET)
extend_qtc_executable(qml2puppet
  CONDITION TARGET Qt::Quick3DAssetUtils
  FEATURE_INFO "Qt Quick 3D asset utils"
  DEPENDS Qt::Quick3DAssetUtilsPrivate
  DEFINES QUICK3D_ASSET_UTILS_MODULE
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
    qt5bakelightsnodeinstanceserver.cpp qt5bakelightsnodeinstanceserver.h
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

if (DEFINED MULTILANGUAGE_SUPPORT_SUBDIRECTORY AND Qt6_VERSION VERSION_GREATER_EQUAL 6.2.1)
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

if (Qt6_VERSION VERSION_GREATER_EQUAL 6.4.0)
    extend_qtc_executable(qml2puppet
      # QT_QML_DEBUG is disabled in release builds, but is necessary to have the preview debug channel
      DEFINES ENABLE_INTERNAL_QML_RUNTIME QT_QML_DEBUG
      PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/qml2puppet/runner
      SOURCES_PREFIX qml2puppet/runner
      SOURCES
        qmlruntime.h qmlruntime.cpp
        qmlconfiguration.h loadwatcher.h
    )

    # Turn the tool into its own self-contained qml module
    qt_add_qml_module(qml2puppet
        URI QmlRuntime.QmlConfiguration
        VERSION 1.0
        RESOURCE_PREFIX "/qt-project.org/imports"
    )
    if (QTC_STATIC_BUILD)
      qt_import_qml_plugins(qml2puppet PATH_TO_SCAN ${SRCDIR})
    endif()
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

# add application icon
# IDE_LOGO_PATH in a default QtCreator build is empty, all icons are found by relative paths
# So we can not use the icon then.
if (TARGET qml2puppet AND NOT "${IDE_LOGO_PATH}" STREQUAL "")
    if (WIN32)
      set(RC_APPLICATION_NAME "${IDE_DISPLAY_NAME}")
      set(RC_VERSION "${IDE_VERSION}.0")
      set(RC_VERSION_STRING "${IDE_VERSION_DISPLAY}")
      set(RC_COPYRIGHT "2008-${IDE_COPYRIGHT_YEAR} The Qt Company Ltd")

      string(REPLACE " " "\\x20" RC_APPLICATION_NAME "${RC_APPLICATION_NAME}")
      string(REPLACE " " "\\x20" RC_COPYRIGHT "${RC_COPYRIGHT}")
      string(REPLACE "." "," RC_VERSION "${RC_VERSION}")

      target_compile_definitions(qml2puppet PRIVATE
        RC_APPLICATION_NAME=${RC_APPLICATION_NAME}
        RC_VERSION=${RC_VERSION}
        RC_VERSION_STRING=${RC_VERSION_STRING}
        RC_COPYRIGHT=${RC_COPYRIGHT}
        RC_ICON_PATH=${IDE_ICON_PATH}
      )

      target_sources(qml2puppet PRIVATE windows_application_icon/qml2puppet.rc)
    endif()
endif()
