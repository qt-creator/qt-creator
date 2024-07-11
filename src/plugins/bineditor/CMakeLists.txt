add_qtc_plugin(BinEditor
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    bineditor_global.h
    bineditortr.h
    bineditorconstants.h
    bineditorplugin.cpp
    bineditorservice.h
    markup.cpp
    markup.h
)
