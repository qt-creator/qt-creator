add_qtc_plugin(DiffEditor
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    diffeditor.cpp diffeditor.h
    diffeditor.qrc
    diffeditor_global.h
    diffeditortr.h
    diffeditorconstants.h
    diffeditorcontroller.cpp diffeditorcontroller.h
    diffeditordocument.cpp diffeditordocument.h
    diffeditorfactory.cpp diffeditorfactory.h
    diffeditoricons.h
    diffeditorplugin.cpp diffeditorplugin.h
    diffeditorwidgetcontroller.cpp diffeditorwidgetcontroller.h
    diffenums.h
    diffutils.cpp diffutils.h
    diffview.cpp diffview.h
    selectabletexteditorwidget.cpp selectabletexteditorwidget.h
    sidebysidediffeditorwidget.cpp sidebysidediffeditorwidget.h
    unifieddiffeditorwidget.cpp unifieddiffeditorwidget.h
  EXPLICIT_MOC diffeditor.h
)
