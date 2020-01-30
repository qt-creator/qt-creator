add_qtc_plugin(Mercurial
  PLUGIN_DEPENDS Core TextEditor VcsBase
  SOURCES
    annotationhighlighter.cpp annotationhighlighter.h
    authenticationdialog.cpp authenticationdialog.h authenticationdialog.ui
    commiteditor.cpp commiteditor.h
    constants.h
    mercurialclient.cpp mercurialclient.h
    mercurialcommitpanel.ui
    mercurialcommitwidget.cpp mercurialcommitwidget.h
    mercurialeditor.cpp mercurialeditor.h
    mercurialplugin.cpp mercurialplugin.h
    mercurialsettings.cpp mercurialsettings.h
    optionspage.cpp optionspage.h optionspage.ui
    revertdialog.cpp revertdialog.h revertdialog.ui
    srcdestdialog.cpp srcdestdialog.h srcdestdialog.ui
)
