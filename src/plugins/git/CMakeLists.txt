add_qtc_plugin(Git
  PLUGIN_DEPENDS Core DiffEditor TextEditor VcsBase
  SOURCES
    annotationhighlighter.cpp annotationhighlighter.h
    branchadddialog.cpp branchadddialog.h
    branchcheckoutdialog.cpp branchcheckoutdialog.h
    branchmodel.cpp branchmodel.h
    branchview.cpp branchview.h
    changeselectiondialog.cpp changeselectiondialog.h
    commitdata.cpp commitdata.h
    gerrit/authenticationdialog.cpp gerrit/authenticationdialog.h
    gerrit/branchcombobox.cpp gerrit/branchcombobox.h
    gerrit/gerritdialog.cpp gerrit/gerritdialog.h
    gerrit/gerritmodel.cpp gerrit/gerritmodel.h
    gerrit/gerritoptionspage.cpp gerrit/gerritoptionspage.h
    gerrit/gerritparameters.cpp gerrit/gerritparameters.h
    gerrit/gerritplugin.cpp gerrit/gerritplugin.h
    gerrit/gerritpushdialog.cpp gerrit/gerritpushdialog.h
    gerrit/gerritremotechooser.cpp gerrit/gerritremotechooser.h
    gerrit/gerritserver.cpp gerrit/gerritserver.h
    git.qrc
    git_global.h
    gittr.h
    gitclient.cpp gitclient.h
    gitconstants.h
    giteditor.cpp giteditor.h
    gitgrep.cpp gitgrep.h
    githighlighters.cpp githighlighters.h
    gitplugin.cpp gitplugin.h
    gitsettings.cpp gitsettings.h
    gitsubmiteditor.cpp gitsubmiteditor.h
    gitsubmiteditorwidget.cpp gitsubmiteditorwidget.h
    gitutils.cpp gitutils.h
    logchangedialog.cpp logchangedialog.h
    mergetool.cpp mergetool.h
    remotedialog.cpp remotedialog.h
    remotemodel.cpp remotemodel.h
    stashdialog.cpp stashdialog.h
)
