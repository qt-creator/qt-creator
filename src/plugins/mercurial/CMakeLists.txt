add_qtc_plugin(Mercurial
  PLUGIN_DEPENDS Core TextEditor VcsBase
  SOURCES
    annotationhighlighter.cpp annotationhighlighter.h
    authenticationdialog.cpp authenticationdialog.h
    commiteditor.cpp commiteditor.h
    constants.h
    mercurialclient.cpp mercurialclient.h
    mercurialcommitwidget.cpp mercurialcommitwidget.h
    mercurialeditor.cpp mercurialeditor.h
    mercurialplugin.cpp mercurialplugin.h
    mercurialsettings.cpp mercurialsettings.h
    mercurialtr.h
    revertdialog.cpp revertdialog.h
    srcdestdialog.cpp srcdestdialog.h
)
