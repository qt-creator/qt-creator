add_qtc_library(ExtensionSystem
  DEPENDS Aggregation Utils
  PUBLIC_DEPENDS Qt::Core Qt::Widgets
  SOURCES
    extensionsystem_global.h
    extensionsystemtr.h
    invoker.cpp invoker.h
    iplugin.cpp iplugin.h
    optionsparser.cpp optionsparser.h
    plugindetailsview.cpp plugindetailsview.h
    pluginerroroverview.cpp pluginerroroverview.h
    pluginerrorview.cpp pluginerrorview.h
    pluginmanager.cpp pluginmanager.h pluginmanager_p.h
    pluginspec.cpp pluginspec.h
    pluginview.cpp pluginview.h
  EXPLICIT_MOC
    pluginmanager.h
  SKIP_AUTOMOC pluginmanager.cpp
)

find_package(Qt6 COMPONENTS Test QUIET)

# If ExtensionSystem was compiled with QtTest, it should provide the test options
# and the test related API, regardless if Qt Creator was compiled with tests or not,
# and regardless if an external plugin is compiled with tests or not.
# ExtensionSystem may not require QtTest in public headers though.
# API with EXTENSIONSYSTEM_TEST_EXPORT (like CppPluginSpec) should only be exported
# if Qt Creator is compiled with tests.
extend_qtc_library(ExtensionSystem
  CONDITION TARGET Qt::Test
  DEPENDS Qt::Test
  PUBLIC_DEFINES EXTENSIONSYSTEM_WITH_TESTOPTION
)

extend_qtc_library(ExtensionSystem
  CONDITION TARGET Nanotrace
  PUBLIC_DEPENDS Nanotrace
)
