add_qtc_plugin(Fossil
  PLUGIN_DEPENDS
    Core TextEditor ProjectExplorer VcsBase
  SOURCES
    annotationhighlighter.cpp annotationhighlighter.h
    branchinfo.h
    commiteditor.cpp commiteditor.h
    configuredialog.cpp configuredialog.h
    constants.h
    fossilclient.cpp fossilclient.h
    fossilcommitwidget.cpp fossilcommitwidget.h
    fossileditor.cpp fossileditor.h
    fossilplugin.cpp
    fossilsettings.cpp fossilsettings.h
    fossiltr.h
    pullorpushdialog.cpp pullorpushdialog.h
    revisioninfo.h
    wizard/fossiljsextension.cpp wizard/fossiljsextension.h
)

qtc_add_resources(Fossil "wizard_resources"
  PREFIX "/fossil"
  FILES
    wizard/projects/vcs/icon.png
    wizard/projects/vcs/icon@2x.png
    wizard/projects/vcs/wizard.json
)
