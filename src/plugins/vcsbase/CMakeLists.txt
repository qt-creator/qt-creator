add_qtc_plugin(VcsBase
  PLUGIN_CLASS VcsPlugin
  PLUGIN_DEPENDS Core DiffEditor ProjectExplorer TextEditor
  PLUGIN_RECOMMENDS CodePaster CppEditor
  SOURCES
    baseannotationhighlighter.cpp baseannotationhighlighter.h
    cleandialog.cpp cleandialog.h
    commonvcssettings.cpp commonvcssettings.h
    diffandloghighlighter.cpp diffandloghighlighter.h
    nicknamedialog.cpp nicknamedialog.h
    submiteditorfile.cpp submiteditorfile.h
    submiteditorwidget.cpp submiteditorwidget.h
    submitfieldwidget.cpp submitfieldwidget.h
    submitfilemodel.cpp submitfilemodel.h
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
    vcschangesview.cpp vcschangesview.h
    vcscommand.cpp vcscommand.h
    vcsenums.h
    vcsfilestatus.h
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

qtc_add_resources(VcsBase "images"
  PREFIX "/vcsbase"
  FILES
    images/diff_arrows.png
    images/diff_arrows@2x.png
    images/diff_documents.png
    images/diff_documents@2x.png
    images/settingscategory_vcs.png
    images/settingscategory_vcs@2x.png
    images/submit_arrow.png
    images/submit_arrow@2x.png
    images/submit_db.png
    images/submit_db@2x.png
)
