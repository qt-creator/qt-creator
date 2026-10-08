add_qtc_plugin(SafeRenderer
  PLUGIN_DEPENDS
    QtCreator::Core QtCreator::ProjectExplorer
  SOURCES
    saferenderer.h
  SKIP_PCH
)

file(GLOB_RECURSE wizards RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} wizards/*)
qtc_add_resources(SafeRenderer "wizards"
  PREFIX "/saferenderer"
  FILES ${wizards}
)
