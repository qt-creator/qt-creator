add_qtc_plugin(Git
  PLUGIN_DEPENDS Core DiffEditor TextEditor VcsBase
  SOURCES
    annotationhighlighter.cpp annotationhighlighter.h
    branchadddialog.cpp branchadddialog.h branchadddialog.ui
    branchcheckoutdialog.cpp branchcheckoutdialog.h branchcheckoutdialog.ui
    branchmodel.cpp branchmodel.h
    branchview.cpp branchview.h
    changeselectiondialog.cpp changeselectiondialog.h changeselectiondialog.ui
    commitdata.cpp commitdata.h
    gerrit/authenticationdialog.cpp gerrit/authenticationdialog.h gerrit/authenticationdialog.ui
    gerrit/branchcombobox.cpp gerrit/branchcombobox.h
    gerrit/gerritdialog.cpp gerrit/gerritdialog.h gerrit/gerritdialog.ui
    gerrit/gerritmodel.cpp gerrit/gerritmodel.h
    gerrit/gerritoptionspage.cpp gerrit/gerritoptionspage.h
    gerrit/gerritparameters.cpp gerrit/gerritparameters.h
    gerrit/gerritplugin.cpp gerrit/gerritplugin.h
    gerrit/gerritpushdialog.cpp gerrit/gerritpushdialog.h gerrit/gerritpushdialog.ui
    gerrit/gerritremotechooser.cpp gerrit/gerritremotechooser.h
    gerrit/gerritserver.cpp gerrit/gerritserver.h
    git.qrc
    gitclient.cpp gitclient.h
    gitconstants.h
    giteditor.cpp giteditor.h
    gitgrep.cpp gitgrep.h
    githighlighters.cpp githighlighters.h
    gitplugin.cpp gitplugin.h
    gitsettings.cpp gitsettings.h
    gitsubmiteditor.cpp gitsubmiteditor.h
    gitsubmiteditorwidget.cpp gitsubmiteditorwidget.h
    gitsubmitpanel.ui
    gitutils.cpp gitutils.h
    logchangedialog.cpp logchangedialog.h
    mergetool.cpp mergetool.h
    remoteadditiondialog.ui
    remotedialog.cpp remotedialog.h remotedialog.ui
    remotemodel.cpp remotemodel.h
    settingspage.cpp settingspage.h settingspage.ui
    stashdialog.cpp stashdialog.h stashdialog.ui
)
