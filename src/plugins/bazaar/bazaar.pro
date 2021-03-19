include(../../qtcreatorplugin.pri)
SOURCES += \
    bazaarclient.cpp \
    bazaarplugin.cpp \
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
    bazaarsettings.h \
    commiteditor.h \
    bazaarcommitwidget.h \
    bazaareditor.h \
    annotationhighlighter.h \
    pullorpushdialog.h \
    branchinfo.h

FORMS += \
    revertdialog.ui \
    bazaarcommitpanel.ui \
    pullorpushdialog.ui \
    uncommitdialog.ui
