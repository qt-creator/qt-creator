add_qtc_plugin(QDoc
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport TextEditor
  LONG_DESCRIPTION_MD QDocDescription.md
  SOURCES
    qdocconfig.cpp
    qdocconfig.h
    qdoceditor.cpp
    qdoceditor.h
    qdoclinter.cpp
    qdoclinter.h
    qdocparser.cpp
    qdocparser.h
    qdocplugin.cpp
    qdocrenderer.cpp
    qdocrenderer.h
    qdoctr.h
)
