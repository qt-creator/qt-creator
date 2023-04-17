add_qtc_plugin(Fossil
  PLUGIN_DEPENDS
    Core TextEditor ProjectExplorer VcsBase
  SOURCES
    annotationhighlighter.cpp annotationhighlighter.h
    branchinfo.h
    commiteditor.cpp commiteditor.h
    configuredialog.cpp configuredialog.h
    constants.h
    fossil.qrc
    fossilclient.cpp fossilclient.h
    fossilcommitwidget.cpp fossilcommitwidget.h
    fossileditor.cpp fossileditor.h
    fossilplugin.cpp fossilplugin.h
    fossilsettings.cpp fossilsettings.h
    fossiltr.h
    pullorpushdialog.cpp pullorpushdialog.h
    revisioninfo.h
    wizard/fossiljsextension.cpp wizard/fossiljsextension.h
)
