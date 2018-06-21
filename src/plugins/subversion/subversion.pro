include(../../qtcreatorplugin.pri)

HEADERS += annotationhighlighter.h \
    subversionplugin.h \
    subversionclient.h \
    subversioncontrol.h \
    settingspage.h \
    subversioneditor.h \
    subversionsubmiteditor.h \
    subversionsettings.h \
    subversionconstants.h

SOURCES += annotationhighlighter.cpp \
    subversionplugin.cpp \
    subversionclient.cpp \
    subversioncontrol.cpp \
    settingspage.cpp \
    subversioneditor.cpp \
    subversionsubmiteditor.cpp \
    subversionsettings.cpp

FORMS += settingspage.ui
