add_qtc_plugin(Bazaar
  PLUGIN_DEPENDS Core TextEditor VcsBase
  SOURCES
    annotationhighlighter.cpp annotationhighlighter.h
    bazaarclient.cpp bazaarclient.h
    bazaarcommitwidget.cpp bazaarcommitwidget.h
    bazaareditor.cpp bazaareditor.h
    bazaarplugin.cpp bazaarplugin.h
    bazaarsettings.cpp bazaarsettings.h
    bazaartr.h
    branchinfo.cpp branchinfo.h
    commiteditor.cpp commiteditor.h
    constants.h
    pullorpushdialog.cpp pullorpushdialog.h
)
