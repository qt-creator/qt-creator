TARGET = Bazaar
TEMPLATE = lib
include(../../qtcreatorplugin.pri)
include(bazaar_dependencies.pri)
SOURCES += \
    bazaarclient.cpp \
    bazaarcontrol.cpp \
    bazaarplugin.cpp \
    optionspage.cpp \
    bazaarsettings.cpp \
    commiteditor.cpp \
    bazaarcommitwidget.cpp \
    bazaareditor.cpp \
    annotationhighlighter.cpp \
    pullorpushdialog.cpp \
    branchinfo.cpp \
    clonewizardpage.cpp \
    clonewizard.cpp \
    cloneoptionspanel.cpp
HEADERS += \
    bazaarclient.h \
    constants.h \
    bazaarcontrol.h \
    bazaarplugin.h \
    optionspage.h \
    bazaarsettings.h \
    commiteditor.h \
    bazaarcommitwidget.h \
    bazaareditor.h \
    annotationhighlighter.h \
    pullorpushdialog.h \
    branchinfo.h \
    clonewizard.h \
    clonewizardpage.h \
    cloneoptionspanel.h
FORMS += \
    optionspage.ui \
    revertdialog.ui \
    bazaarcommitpanel.ui \
    pullorpushdialog.ui \
    cloneoptionspanel.ui
RESOURCES += bazaar.qrc
