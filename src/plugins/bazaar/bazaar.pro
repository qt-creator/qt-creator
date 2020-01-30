include(../../qtcreatorplugin.pri)
SOURCES += \
    bazaarclient.cpp \
    bazaarplugin.cpp \
    optionspage.cpp \
    bazaarsettings.cpp \
    commiteditor.cpp \
    bazaarcommitwidget.cpp \
    bazaareditor.cpp \
    annotationhighlighter.cpp \
    pullorpushdialog.cpp \
    branchinfo.cpp

HEADERS += \
    bazaarclient.h \
    constants.h \
    bazaarplugin.h \
    optionspage.h \
    bazaarsettings.h \
    commiteditor.h \
    bazaarcommitwidget.h \
    bazaareditor.h \
    annotationhighlighter.h \
    pullorpushdialog.h \
    branchinfo.h

FORMS += \
    optionspage.ui \
    revertdialog.ui \
    bazaarcommitpanel.ui \
    pullorpushdialog.ui \
    uncommitdialog.ui
