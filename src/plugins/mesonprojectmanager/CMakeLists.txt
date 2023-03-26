add_qtc_plugin(MesonProjectManager
  PLUGIN_CLASS MesonProjectPlugin
  DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer TextEditor QtSupport
  SOURCES
  mesonprojectplugin.cpp
  mesonprojectplugin.h
  versionhelper.h
  mesonactionsmanager.h
  mesonactionsmanager.cpp
  toolsmodel.cpp
  toolssettingswidget.h
  toolssettingswidget.cpp
  toolssettingspage.cpp
  toolssettingspage.h
  toolitemsettings.cpp
  toolitemsettings.h
  tooltreeitem.cpp
  tooltreeitem.h
  toolsmodel.h
  ninjatoolkitaspect.cpp
  ninjatoolkitaspect.h
  toolkitaspectwidget.h
  toolkitaspectwidget.cpp
  mesontoolkitaspect.cpp
  mesontoolkitaspect.h
  toolssettingsaccessor.h
  toolssettingsaccessor.cpp
  settings.h
  settings.cpp
  mesonwrapper.cpp
  mesonwrapper.h
  ninjawrapper.h
  toolwrapper.h
  toolwrapper.cpp
  mesontools.h
  mesontools.cpp
  mesoninfoparser.h
  buildoptions.h
  target.h
  mesoninfo.h
  common.h
  buildoptionsparser.h
  buildsystemfilesparser.h
  infoparser.h
  targetparser.h
  kitdata.h
  kithelper.h
  mesonproject.h
  mesonproject.cpp
  mesonprojectimporter.h
  mesonprojectimporter.cpp
  mesonbuildsystem.h
  mesonbuildsystem.cpp
  mesonprojectparser.h
  mesonprojectparser.cpp
  mesonbuildconfiguration.h
  mesonbuildconfiguration.cpp
  ninjabuildstep.h
  ninjabuildstep.cpp
  mesonbuildsettingswidget.h
  mesonbuildsettingswidget.cpp
  buildoptionsmodel.h
  buildoptionsmodel.cpp
  arrayoptionlineedit.cpp
  arrayoptionlineedit.h
  mesonprocess.h
  mesonprocess.cpp
  mesonoutputparser.h
  mesonoutputparser.cpp
  ninjaparser.h
  ninjaparser.cpp
  mesonrunconfiguration.h
  mesonrunconfiguration.cpp
  projecttree.h
  projecttree.cpp
  mesonprojectnodes.h
  mesonprojectnodes.cpp
  machinefilemanager.h
  machinefilemanager.cpp
  nativefilegenerator.h
  nativefilegenerator.cpp
  resources_meson.qrc
)

file(RELATIVE_PATH RELATIVE_TEST_PATH "${PROJECT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
file(RELATIVE_PATH TEST_RELATIVE_LIBEXEC_PATH "/${RELATIVE_TEST_PATH}" "/${IDE_LIBEXEC_PATH}")

if(WITH_TESTS)
add_qtc_test(tst_mesonwrapper
  EXCLUDE_FROM_PRECHECK
  INCLUDES
    BEFORE "."
  DEPENDS
    Qt::Core Qt::Test Core
    Utils
  DEFINES
     MESON_SAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/resources"
     MESON_SAMPLES_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
     TEST_RELATIVE_LIBEXEC_PATH="${TEST_RELATIVE_LIBEXEC_PATH}"
  SOURCES
    tests/testmesonwrapper.cpp
    mesonwrapper.cpp
    mesonwrapper.h
    ninjawrapper.h
    toolwrapper.h
    toolwrapper.cpp
    mesontools.h
)

add_qtc_test(tst_mesoninfoparser
  EXCLUDE_FROM_PRECHECK
  INCLUDES
    BEFORE "."
  DEPENDS
    Qt::Core Qt::Test Core
    Utils
  DEFINES
     MESON_SAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/resources"
     MESON_SAMPLES_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
     TEST_RELATIVE_LIBEXEC_PATH="${TEST_RELATIVE_LIBEXEC_PATH}"
  SOURCES
    tests/testmesoninfoparser.cpp
    mesonwrapper.cpp
    mesonwrapper.h
    ninjawrapper.h
    toolwrapper.h
    toolwrapper.cpp
    mesontools.h
    mesoninfoparser.h
)

add_qtc_test(tst_ninjaparser
  INCLUDES
    BEFORE "."
  DEPENDS
    Qt::Core Qt::Test Core
    Utils ProjectExplorer
  SOURCES
    tests/testninjaparser.cpp
    ninjaparser.cpp
)

add_qtc_test(tst_mesonparser
  INCLUDES
    BEFORE "."
  DEFINES
    MESONPARSER_DISABLE_TASKS_FOR_TESTS
  DEPENDS
    Qt::Core Qt::Test Core
    Utils ProjectExplorer
  SOURCES
    tests/testmesonparser.cpp
    mesonoutputparser.cpp
)


endif(WITH_TESTS)
