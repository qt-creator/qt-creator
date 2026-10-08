add_qtc_plugin(DiffEditor
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    diffeditor.cpp diffeditor.h
    diffeditor_global.h
    diffeditortr.h
    diffeditorconstants.h
    diffeditorcontroller.cpp diffeditorcontroller.h
    diffeditordocument.cpp diffeditordocument.h
    diffeditoricons.h
    diffeditorplugin.cpp
    diffeditorwidgetcontroller.cpp diffeditorwidgetcontroller.h
    diffenums.h
    diffutils.cpp diffutils.h
    inlinediff.cpp inlinediff.h
    inlinediff_p.h
    selectabletexteditorwidget.cpp selectabletexteditorwidget.h
    sidebysidediffeditorwidget.cpp sidebysidediffeditorwidget.h
    unifieddiffeditorwidget.cpp unifieddiffeditorwidget.h
)

qtc_add_resources(DiffEditor "images"
  PREFIX "/diffeditor"
  FILES
    images/sidebysidediff.png
    images/sidebysidediff@2x.png
    images/unifieddiff.png
    images/unifieddiff@2x.png
    images/topbar.png
    images/topbar@2x.png
)
