TEMPLATE = lib
TARGET = VCSBase
DEFINES += VCSBASE_LIBRARY
include(../../qtcreatorplugin.pri)
include(vcsbase_dependencies.pri)
HEADERS += vcsbase_global.h \
    vcsbaseconstants.h \
    vcsplugin.h \
    corelistener.h \
    vcsbaseplugin.h \
    baseannotationhighlighter.h \
    diffhighlighter.h \
    vcsbasetextdocument.h \
    vcsbaseeditor.h \
    vcsbasesubmiteditor.h \
    basevcseditorfactory.h \
    submiteditorfile.h \
    basevcssubmiteditorfactory.h \
    submitfilemodel.h \
    vcsbasesettings.h \
    vcsbasesettingspage.h \
    nicknamedialog.h \
    basecheckoutwizard.h \
    checkoutwizarddialog.h \
    checkoutprogresswizardpage.h \
    checkoutjobs.h \
    basecheckoutwizardpage.h \
    vcsbaseoutputwindow.h

SOURCES += vcsplugin.cpp \
    vcsbaseplugin.cpp \
    corelistener.cpp \
    baseannotationhighlighter.cpp \
    diffhighlighter.cpp \
    vcsbasetextdocument.cpp \
    vcsbaseeditor.cpp \
    vcsbasesubmiteditor.cpp \
    basevcseditorfactory.cpp \
    submiteditorfile.cpp \
    basevcssubmiteditorfactory.cpp \
    submitfilemodel.cpp \
    vcsbasesettings.cpp \
    vcsbasesettingspage.cpp \
    nicknamedialog.cpp \
    basecheckoutwizard.cpp \
    checkoutwizarddialog.cpp \
    checkoutprogresswizardpage.cpp \
    checkoutjobs.cpp \
    basecheckoutwizardpage.cpp \
    vcsbaseoutputwindow.cpp

RESOURCES += vcsbase.qrc

FORMS += vcsbasesettingspage.ui \
    nicknamedialog.ui \
    checkoutprogresswizardpage.ui \
    basecheckoutwizardpage.ui

OTHER_FILES += VCSBase.pluginspec
