TEMPLATE = lib
TARGET = CppTools
include(../../qtcreatorplugin.pri)
include(../../plugins/locator/locator.pri)
include(cpptools_dependencies.pri)

# DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
INCLUDEPATH += .
DEFINES += CPPTOOLS_LIBRARY
HEADERS += completionsettingspage.h \
    cppclassesfilter.h \
    cppcodecompletion.h \
    cppcurrentdocumentfilter.h \
    cppfunctionsfilter.h \
    cppmodelmanager.h \
    cppmodelmanagerinterface.h \
    cpplocatorfilter.h \
    cpptools_global.h \
    cpptoolsconstants.h \
    cpptoolseditorsupport.h \
    cpptoolsplugin.h \
    searchsymbols.h \
    cppdoxygen.h \
    cppfilesettingspage.h \
    cppfindreferences.h

SOURCES += completionsettingspage.cpp \
    cppclassesfilter.cpp \
    cppcodecompletion.cpp \
    cppcurrentdocumentfilter.cpp \
    cppfunctionsfilter.cpp \
    cppmodelmanager.cpp \
    cpplocatorfilter.cpp \
    cpptoolseditorsupport.cpp \
    cpptoolsplugin.cpp \
    searchsymbols.cpp \
    cppdoxygen.cpp \
    cppfilesettingspage.cpp \
    abstracteditorsupport.cpp \
    cppfindreferences.cpp

FORMS += completionsettingspage.ui \
    cppfilesettingspage.ui

OTHER_FILES += CppTools.pluginspec
