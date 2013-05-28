include(../../qtcreatorplugin.pri)

HEADERS += annotationhighlighter.h \
    subversionplugin.h \
    subversioncontrol.h \
    settingspage.h \
    subversioneditor.h \
    subversionsubmiteditor.h \
    subversionsettings.h \
    checkoutwizard.h \
    checkoutwizardpage.h \
    subversionconstants.h

SOURCES += annotationhighlighter.cpp \
    subversionplugin.cpp \
    subversioncontrol.cpp \
    settingspage.cpp \
    subversioneditor.cpp \
    subversionsubmiteditor.cpp \
    subversionsettings.cpp \
    checkoutwizard.cpp \
    checkoutwizardpage.cpp

FORMS += settingspage.ui

RESOURCES += subversion.qrc
