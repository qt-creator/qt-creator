add_qtc_plugin(Coco
  PUBLIC_DEPENDS app_version
  PLUGIN_DEPENDS Core LanguageClient
  SOURCES
    cocolanguageclient.cpp cocolanguageclient.h
    cocoplugin.cpp cocoplugin.h
    cocotr.h
)
