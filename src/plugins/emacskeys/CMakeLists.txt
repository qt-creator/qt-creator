add_qtc_plugin(EmacsKeys
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    emacskeysconstants.h
    emacskeysplugin.cpp emacskeysplugin.h
    emacskeysstate.cpp emacskeysstate.h
)
