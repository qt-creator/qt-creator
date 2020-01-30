include(../../qtcreatorplugin.pri)
SOURCES += mercurialplugin.cpp \
    optionspage.cpp \
    mercurialclient.cpp \
    annotationhighlighter.cpp \
    mercurialeditor.cpp \
    revertdialog.cpp \
    srcdestdialog.cpp \
    mercurialcommitwidget.cpp \
    commiteditor.cpp \
    mercurialsettings.cpp \
    authenticationdialog.cpp
HEADERS += mercurialplugin.h \
    constants.h \
    optionspage.h \
    mercurialclient.h \
    annotationhighlighter.h \
    mercurialeditor.h \
    revertdialog.h \
    srcdestdialog.h \
    mercurialcommitwidget.h \
    commiteditor.h \
    mercurialsettings.h \
    authenticationdialog.h
FORMS += optionspage.ui \
    revertdialog.ui \
    srcdestdialog.ui \
    mercurialcommitpanel.ui \
    authenticationdialog.ui
