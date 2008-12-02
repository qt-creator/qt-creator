TEMPLATE = lib
TARGET = Subversion

include(../../qworkbenchplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/vcsbase/vcsbase.pri)
include(../../libs/utils/utils.pri)

HEADERS += annotationhighlighter.h \
    subversionplugin.h \
    subversioncontrol.h \
    subversionoutputwindow.h \
    settingspage.h \
    subversioneditor.h \
    changenumberdialog.h \
    subversionsubmiteditor.h \
    subversionsettings.h

SOURCES += annotationhighlighter.cpp \
    subversionplugin.cpp \
    subversioncontrol.cpp \
    subversionoutputwindow.cpp \
    settingspage.cpp \
    subversioneditor.cpp \
    changenumberdialog.cpp \
    subversionsubmiteditor.cpp \
    subversionsettings.cpp

FORMS += settingspage.ui \
    changenumberdialog.ui

RESOURCES += subversion.qrc
