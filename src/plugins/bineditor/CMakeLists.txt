add_qtc_plugin(BinEditor
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    bineditor_global.h
    bineditorconstants.h
    bineditorplugin.cpp bineditorplugin.h
    bineditorservice.h
    bineditorwidget.cpp bineditorwidget.h
    markup.cpp markup.h
)
