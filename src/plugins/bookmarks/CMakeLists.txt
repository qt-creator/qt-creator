add_qtc_plugin(Bookmarks
  PLUGIN_DEPENDS Core ProjectExplorer TextEditor
  SOURCES
    bookmark.cpp bookmark.h
    bookmarkfilter.cpp bookmarkfilter.h
    bookmarkmanager.cpp bookmarkmanager.h
    bookmarks_global.h
    bookmarksplugin.cpp bookmarksplugin.h
)
