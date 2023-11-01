add_qtc_plugin(MesonProjectManager
  PLUGIN_CLASS MesonProjectPlugin
  DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer TextEditor QtSupport
  SOURCES
  arrayoptionlineedit.cpp
  arrayoptionlineedit.h
  buildoptions.h
  buildoptionsmodel.cpp
  buildoptionsmodel.h
  buildoptionsparser.h
  buildsystemfilesparser.h
  common.h
  infoparser.h
  kitdata.h
  mesonactionsmanager.cpp
  mesonactionsmanager.h
  mesonbuildconfiguration.cpp
  mesonbuildconfiguration.h
  mesonbuildsystem.cpp
  mesonbuildsystem.h
  mesoninfo.h
  mesoninfoparser.h
  mesonoutputparser.cpp
  mesonoutputparser.h
  mesonproject.cpp
  mesonproject.h
  mesonprojectimporter.cpp
  mesonprojectimporter.h
  mesonprojectmanagertr.h
  mesonprojectnodes.cpp
  mesonprojectnodes.h
  mesonprojectparser.cpp
  mesonprojectparser.h
  mesonprojectplugin.cpp
  mesonrunconfiguration.cpp
  mesonrunconfiguration.h
  mesontoolkitaspect.cpp
  mesontoolkitaspect.h
  mesontools.cpp
  mesontools.h
  mesonwrapper.cpp
  mesonwrapper.h
  ninjabuildstep.cpp
  ninjabuildstep.h
  ninjaparser.cpp
  ninjaparser.h
  ninjatoolkitaspect.cpp
  ninjatoolkitaspect.h
  ninjawrapper.h
  projecttree.cpp
  projecttree.h
  resources_meson.qrc
  settings.cpp
  settings.h
  target.h
  targetparser.h
  toolitemsettings.cpp
  toolitemsettings.h
  toolkitaspectwidget.cpp
  toolkitaspectwidget.h
  toolsmodel.cpp
  toolsmodel.h
  toolssettingsaccessor.cpp
  toolssettingsaccessor.h
  toolssettingspage.cpp
  toolssettingspage.h
  tooltreeitem.cpp
  tooltreeitem.h
  toolwrapper.cpp
  toolwrapper.h
  versionhelper.h
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
