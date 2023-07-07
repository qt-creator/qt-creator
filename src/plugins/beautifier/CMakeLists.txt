add_qtc_plugin(Beautifier
  DEPENDS Qt::Xml
  PLUGIN_DEPENDS Core ProjectExplorer TextEditor
  SOURCES
    abstractsettings.cpp abstractsettings.h
    artisticstyle/artisticstyle.cpp artisticstyle/artisticstyle.h
    beautifier.qrc
    beautifierabstracttool.h
    beautifierconstants.h
    beautifierplugin.cpp beautifierplugin.h
    beautifiertr.h
    clangformat/clangformat.cpp clangformat/clangformat.h
    configurationdialog.cpp configurationdialog.h
    configurationeditor.cpp configurationeditor.h
    configurationpanel.cpp configurationpanel.h
    generalsettings.cpp generalsettings.h
    uncrustify/uncrustify.cpp uncrustify/uncrustify.h
)
