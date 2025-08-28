add_qtc_plugin(MesonProjectManager
  PLUGIN_CLASS MesonProjectPlugin
  DEPENDS QmlJS
  PLUGIN_DEPENDS Core ProjectExplorer TextEditor QtSupport
  SOURCES
  buildoptions.h
  buildoptionsmodel.cpp
  buildoptionsmodel.h
  common.h
  kitdata.h
  mesonactionsmanager.cpp
  mesonactionsmanager.h
  mesonbuildconfiguration.cpp
  mesonbuildconfiguration.h
  mesonbuildstep.cpp
  mesonbuildstep.h
  mesonbuildsystem.cpp
  mesonbuildsystem.h
  mesoninfoparser.cpp
  mesoninfoparser.h
  mesonoutputparser.cpp
  mesonoutputparser.h
  mesonpluginconstants.h
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
  mesontools.cpp
  mesontools.h
  ninjaparser.cpp
  ninjaparser.h
  settings.cpp
  settings.h
  target.h
  toolkitaspectwidget.cpp
  toolkitaspectwidget.h
  toolsmodel.cpp
  toolsmodel.h
  toolssettingsaccessor.cpp
  toolssettingsaccessor.h
  toolssettingspage.cpp
  toolssettingspage.h
)

qtc_add_resources(MesonProjectManager "images"
  PREFIX "/mesonproject"
  BASE "."
  FILES
    icons/meson_logo.png
    icons/meson_bw_logo.png
    icons/meson_bw_logo@2x.png
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
    mesontools.cpp
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
    mesoninfoparser.cpp
    mesoninfoparser.h
    mesontools.cpp
    mesontools.h
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
