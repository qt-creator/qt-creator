add_qtc_plugin(VcsBase
  PLUGIN_CLASS VcsPlugin
  PLUGIN_DEPENDS Core DiffEditor ProjectExplorer TextEditor
  PLUGIN_RECOMMENDS CodePaster CppEditor
  SOURCES
    baseannotationhighlighter.cpp baseannotationhighlighter.h
    basevcseditorfactory.cpp basevcseditorfactory.h
    basevcssubmiteditorfactory.cpp basevcssubmiteditorfactory.h
    cleandialog.cpp cleandialog.h
    commonvcssettings.cpp commonvcssettings.h
    diffandloghighlighter.cpp diffandloghighlighter.h
    nicknamedialog.cpp nicknamedialog.h
    submiteditorfile.cpp submiteditorfile.h
    submiteditorwidget.cpp submiteditorwidget.h
    submitfieldwidget.cpp submitfieldwidget.h
    submitfilemodel.cpp submitfilemodel.h
    vcsbase.qrc
    vcsbase_global.h
    vcsbasetr.h
    vcsbaseclient.cpp vcsbaseclient.h
    vcsbaseclientsettings.cpp vcsbaseclientsettings.h
    vcsbaseconstants.h
    vcsbasediffeditorcontroller.cpp vcsbasediffeditorcontroller.h
    vcsbaseeditor.cpp vcsbaseeditor.h
    vcsbaseeditorconfig.cpp vcsbaseeditorconfig.h
    vcsbaseplugin.cpp vcsbaseplugin.h
    vcsbasesubmiteditor.cpp vcsbasesubmiteditor.h
    vcscommand.cpp vcscommand.h
    vcsenums.h
    vcsoutputformatter.cpp vcsoutputformatter.h
    vcsoutputwindow.cpp vcsoutputwindow.h
    vcsplugin.cpp vcsplugin.h
    wizard/vcscommandpage.cpp wizard/vcscommandpage.h
    wizard/vcsconfigurationpage.cpp wizard/vcsconfigurationpage.h
    wizard/vcsjsextension.cpp wizard/vcsjsextension.h
)

extend_qtc_plugin(VcsBase
  CONDITION WITH_TESTS
  DEFINES SRC_DIR="${PROJECT_SOURCE_DIR}"
)
