DEFINES += VCSBASE_LIBRARY
include(../../qtcreatorplugin.pri)
HEADERS += vcsbase_global.h \
    vcsbaseconstants.h \
    vcsprojectcache.h \
    wizard/vcsconfigurationpage.h \
    wizard/vcsjsextension.h \
    vcsplugin.h \
    corelistener.h \
    vcsbaseplugin.h \
    baseannotationhighlighter.h \
    diffandloghighlighter.h \
    vcsbaseeditor.h \
    vcsbasesubmiteditor.h \
    basevcseditorfactory.h \
    submiteditorfile.h \
    basevcssubmiteditorfactory.h \
    submitfilemodel.h \
    commonvcssettings.h \
    commonsettingspage.h \
    nicknamedialog.h \
    basecheckoutwizardfactory.h \
    basecheckoutwizard.h \
    checkoutprogresswizardpage.h \
    basecheckoutwizardpage.h \
    vcsoutputwindow.h \
    cleandialog.h \
    vcsbaseoptionspage.h \
    vcscommand.h \
    vcsbaseclient.h \
    vcsbaseclientsettings.h \
    vcsbaseeditorparameterwidget.h \
    submitfieldwidget.h \
    submiteditorwidget.h

SOURCES += vcsplugin.cpp \
    vcsbaseplugin.cpp \
    vcsprojectcache.cpp \
    wizard/vcsconfigurationpage.cpp \
    wizard/vcsjsextension.cpp \
    corelistener.cpp \
    baseannotationhighlighter.cpp \
    diffandloghighlighter.cpp \
    vcsbaseeditor.cpp \
    vcsbasesubmiteditor.cpp \
    basevcseditorfactory.cpp \
    submiteditorfile.cpp \
    basevcssubmiteditorfactory.cpp \
    submitfilemodel.cpp \
    commonvcssettings.cpp \
    commonsettingspage.cpp \
    nicknamedialog.cpp \
    basecheckoutwizardfactory.cpp \
    basecheckoutwizard.cpp \
    checkoutprogresswizardpage.cpp \
    basecheckoutwizardpage.cpp \
    vcsoutputwindow.cpp \
    cleandialog.cpp \
    vcsbaseoptionspage.cpp \
    vcscommand.cpp \
    vcsbaseclient.cpp \
    vcsbaseclientsettings.cpp \
    vcsbaseeditorparameterwidget.cpp \
    submitfieldwidget.cpp \
    submiteditorwidget.cpp

RESOURCES += vcsbase.qrc

FORMS += commonsettingspage.ui \
    nicknamedialog.ui \
    basecheckoutwizardpage.ui \
    cleandialog.ui \
    submiteditorwidget.ui
