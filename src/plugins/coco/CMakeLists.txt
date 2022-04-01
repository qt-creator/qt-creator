add_qtc_plugin(Coco
  PUBLIC_DEPENDS app_version
  PLUGIN_DEPENDS Core LanguageClient
  SOURCES
    cocoplugin.cpp cocoplugin.h
    cocolanguageclient.cpp cocolanguageclient.h
)
