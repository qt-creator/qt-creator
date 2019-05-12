add_qtc_plugin(ClassView
  PLUGIN_DEPENDS Core CppTools ProjectExplorer TextEditor
  SOURCES
    classviewconstants.h
    classviewmanager.cpp classviewmanager.h
    classviewnavigationwidget.cpp classviewnavigationwidget.h
    classviewnavigationwidgetfactory.cpp classviewnavigationwidgetfactory.h
    classviewparser.cpp classviewparser.h
    classviewparsertreeitem.cpp classviewparsertreeitem.h
    classviewplugin.cpp classviewplugin.h
    classviewsymbolinformation.cpp classviewsymbolinformation.h
    classviewsymbollocation.cpp classviewsymbollocation.h
    classviewtreeitemmodel.cpp classviewtreeitemmodel.h
    classviewutils.cpp classviewutils.h
)
