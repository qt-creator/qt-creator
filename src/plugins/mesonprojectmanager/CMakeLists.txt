add_qtc_plugin(MesonProjectManager
  DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppTools ProjectExplorer TextEditor QtSupport
  PLUGIN_RECOMMENDS Designer
  SOURCES
  mesonprojectplugin.cpp
  mesonprojectplugin.h
  versionhelper.h
  mesonactionsmanager/mesonactionsmanager.h
  mesonactionsmanager/mesonactionsmanager.cpp
  settings/tools/toolsmodel.cpp
  settings/tools/toolssettingswidget.h
  settings/tools/toolssettingswidget.ui
  settings/tools/toolssettingswidget.cpp
  settings/tools/toolssettingspage.cpp
  settings/tools/toolssettingspage.h
  settings/tools/toolitemsettings.ui
  settings/tools/toolitemsettings.cpp
  settings/tools/toolitemsettings.h
  settings/tools/tooltreeitem.cpp
  settings/tools/tooltreeitem.h
  settings/tools/toolsmodel.h
  settings/tools/kitaspect/ninjatoolkitaspect.cpp
  settings/tools/kitaspect/ninjatoolkitaspect.h
  settings/tools/kitaspect/toolkitaspectwidget.h
  settings/tools/kitaspect/toolkitaspectwidget.cpp
  settings/tools/kitaspect/mesontoolkitaspect.cpp
  settings/tools/kitaspect/mesontoolkitaspect.h
  settings/tools/toolssettingsaccessor.h
  settings/tools/toolssettingsaccessor.cpp
  settings/general/generalsettingswidget.ui
  settings/general/generalsettingswidget.cpp
  settings/general/generalsettingswidget.h
  settings/general/generalsettingspage.h
  settings/general/generalsettingspage.cpp
  settings/general/settings.h
  settings/general/settings.cpp
  exewrappers/mesonwrapper.cpp
  exewrappers/mesonwrapper.h
  exewrappers/ninjawrapper.h
  exewrappers/toolwrapper.h
  exewrappers/toolwrapper.cpp
  exewrappers/mesontools.h
  exewrappers/mesontools.cpp
  mesoninfoparser/mesoninfoparser.h
  mesoninfoparser/buildoptions.h
  mesoninfoparser/target.h
  mesoninfoparser/mesoninfo.h
  mesoninfoparser/parsers/common.h
  mesoninfoparser/parsers/buildoptionsparser.h
  mesoninfoparser/parsers/buildsystemfilesparser.h
  mesoninfoparser/parsers/infoparser.h
  mesoninfoparser/parsers/targetparser.h
  kithelper/kitdata.h
  kithelper/kithelper.h
  project/mesonproject.h
  project/mesonproject.cpp
  project/mesonprojectimporter.h
  project/mesonprojectimporter.cpp
  project/mesonbuildsystem.h
  project/mesonbuildsystem.cpp
  project/mesonprojectparser.h
  project/mesonprojectparser.cpp
  project/mesonbuildconfiguration.h
  project/mesonbuildconfiguration.cpp
  project/ninjabuildstep.h
  project/ninjabuildstep.cpp
  project/buildoptions/mesonbuildstepconfigwidget.ui
  project/buildoptions/mesonbuildstepconfigwidget.h
  project/buildoptions/mesonbuildstepconfigwidget.cpp
  project/buildoptions/mesonbuildsettingswidget.ui
  project/buildoptions/mesonbuildsettingswidget.h
  project/buildoptions/mesonbuildsettingswidget.cpp
  project/buildoptions/optionsmodel/buildoptionsmodel.h
  project/buildoptions/optionsmodel/buildoptionsmodel.cpp
  project/buildoptions/optionsmodel/arrayoptionlineedit.cpp
  project/buildoptions/optionsmodel/arrayoptionlineedit.h
  project/mesonprocess.h
  project/mesonprocess.cpp
  project/outputparsers/mesonoutputparser.h
  project/outputparsers/mesonoutputparser.cpp
  project/outputparsers/ninjaparser.h
  project/outputparsers/ninjaparser.cpp
  project/mesonrunconfiguration.h
  project/mesonrunconfiguration.cpp
  project/projecttree/projecttree.h
  project/projecttree/projecttree.cpp
  project/projecttree/mesonprojectnodes.h
  project/projecttree/mesonprojectnodes.cpp
  machinefiles/machinefilemanager.h
  machinefiles/machinefilemanager.cpp
  machinefiles/nativefilegenerator.h
  machinefiles/nativefilegenerator.cpp
  resources.qrc
)

if(WITH_TESTS)
add_qtc_test(tst_mesonwrapper
  INCLUDES
    BEFORE "."
  DEPENDS
    Qt5::Core Qt5::Test Core
    Utils
  DEFINES
     MESON_SAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/resources"
     MESON_SAMPLES_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
  SOURCES
    tests/testmesonwrapper.cpp
    exewrappers/mesonwrapper.cpp
    exewrappers/mesonwrapper.h
    exewrappers/ninjawrapper.h
    exewrappers/toolwrapper.h
    exewrappers/toolwrapper.cpp
    exewrappers/mesontools.h
)

add_qtc_test(tst_mesoninfoparser
  INCLUDES
    BEFORE "."
  DEPENDS
    Qt5::Core Qt5::Test Core
    Utils
  DEFINES
     MESON_SAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/resources"
     MESON_SAMPLES_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
  SOURCES
    tests/testmesoninfoparser.cpp
    exewrappers/mesonwrapper.cpp
    exewrappers/mesonwrapper.h
    exewrappers/ninjawrapper.h
    exewrappers/toolwrapper.h
    exewrappers/toolwrapper.cpp
    exewrappers/mesontools.h
    mesoninfoparser/mesoninfoparser.h
)

add_qtc_test(tst_ninjaparser
  INCLUDES
    BEFORE "."
  DEPENDS
    Qt5::Core Qt5::Test Core
    Utils ProjectExplorer
  SOURCES
    tests/testninjaparser.cpp
    project/outputparsers/ninjaparser.cpp
)

add_qtc_test(tst_mesonparser
  INCLUDES
    BEFORE "."
  DEFINES
    MESONPARSER_DISABLE_TASKS_FOR_TESTS
  DEPENDS
    Qt5::Core Qt5::Test Core
    Utils ProjectExplorer
  SOURCES
    tests/testmesonparser.cpp
    project/outputparsers/mesonoutputparser.cpp
)


endif(WITH_TESTS)
