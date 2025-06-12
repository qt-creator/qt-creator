@PACKAGE_INIT@

# Qt version used for building
# A compatible Qt version should be used by externally built plugins
set(QTC_QT_VERSION "@Qt6_VERSION@")

# force plugins to same path naming conventions as Qt Creator
# otherwise plugins will not be found
if(UNIX AND NOT APPLE)
  include(GNUInstallDirs)
  set(CMAKE_INSTALL_BINDIR "@CMAKE_INSTALL_BINDIR@")
  set(CMAKE_INSTALL_LIBDIR "@CMAKE_INSTALL_LIBDIR@")
  set(CMAKE_INSTALL_LIBEXECDIR "@CMAKE_INSTALL_LIBEXECDIR@")
  set(CMAKE_INSTALL_DATAROOTDIR "@CMAKE_INSTALL_DATAROOTDIR@")
endif()

include(CMakeFindDependencyMacro)
find_dependency(Qt6 "@IDE_QT_VERSION_MIN@"
  REQUIRED COMPONENTS
  Concurrent Core Gui Widgets Core5Compat Network PrintSupport Qml Sql
)
find_dependency(Qt6 COMPONENTS Quick QuickWidgets QUIET)

if (NOT IDE_VERSION)
  include("${CMAKE_CURRENT_LIST_DIR}/QtCreatorIDEBranding.cmake")
endif()

if (NOT DEFINED add_qtc_plugin)
  include("${CMAKE_CURRENT_LIST_DIR}/QtCreatorAPI.cmake")
endif()

if (NOT DEFINED add_translation_targets)
  include("${CMAKE_CURRENT_LIST_DIR}/QtCreatorTranslations.cmake")
endif()

if (NOT DEFINED add_qtc_documentation)
  include("${CMAKE_CURRENT_LIST_DIR}/QtCreatorDocumentation.cmake")
endif()

if (NOT TARGET QtCreator::Core)
  include("${CMAKE_CURRENT_LIST_DIR}/QtCreatorTargets.cmake")
endif()
