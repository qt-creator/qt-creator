TEMPLATE = lib
TARGET = ScmGit
include(../../qworkbenchplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/vcsbase/vcsbase.pri)
include(../../libs/utils/utils.pri)

HEADERS += gitplugin.h \
    gitconstants.h \
    gitoutputwindow.h \
    gitclient.h \
    changeselectiondialog.h \
    commitdata.h \
    settingspage.h \
    giteditor.h \
    annotationhighlighter.h \
    gitsubmiteditorwidget.h \
    gitsubmiteditor.h \
    gitversioncontrol.h

SOURCES += gitplugin.cpp \
    gitoutputwindow.cpp \
    gitclient.cpp \
    changeselectiondialog.cpp \
    commitdata.cpp \
    settingspage.cpp \
    giteditor.cpp \
    annotationhighlighter.cpp \
    gitsubmiteditorwidget.cpp \
    gitsubmiteditor.cpp \
    gitversioncontrol.cpp

FORMS += changeselectiondialog.ui \
    settingspage.ui \
    gitsubmitpanel.ui
