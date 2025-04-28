add_qtc_plugin(SafeRenderer
  PLUGIN_DEPENDS
    QtCreator::Core QtCreator::ProjectExplorer
  SOURCES
    saferenderer.qrc
    saferenderer.h
  SKIP_PCH
)
