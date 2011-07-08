TEMPLATE = lib
TARGET = CppTools
include(../../qtcreatorplugin.pri)
include($$IDE_SOURCE_TREE/src/plugins/locator/locator.pri)
include(cpptools_dependencies.pri)

# DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
INCLUDEPATH += .
DEFINES += CPPTOOLS_LIBRARY
HEADERS += completionsettingspage.h \
    cppclassesfilter.h \
    cppcurrentdocumentfilter.h \
    cppfunctionsfilter.h \
    cppmodelmanager.h \
    cpplocatorfilter.h \
    cpptools_global.h \
    cpptoolsconstants.h \
    cpptoolseditorsupport.h \
    cpptoolsplugin.h \
    cppqtstyleindenter.h \
    searchsymbols.h \
    cppdoxygen.h \
    cppfilesettingspage.h \
    cppfindreferences.h \
    cppcodeformatter.h \
    symbolsfindfilter.h \
    uicodecompletionsupport.h \
    insertionpointlocator.h \
    cpprefactoringchanges.h \
    abstracteditorsupport.h \
    cppcompletionassist.h \
    cppcodestylesettingspage.h \
    cpptoolssettings.h \
    cppcodestylesettings.h \
    cppcodestylesettingsfactory.h \
    cppcodestylepreferences.h \
    cpptoolsreuse.h

SOURCES += completionsettingspage.cpp \
    cppclassesfilter.cpp \
    cppcurrentdocumentfilter.cpp \
    cppfunctionsfilter.cpp \
    cppmodelmanager.cpp \
    cpplocatorfilter.cpp \
    cpptoolseditorsupport.cpp \
    cpptoolsplugin.cpp \
    cppqtstyleindenter.cpp \
    searchsymbols.cpp \
    cppdoxygen.cpp \
    cppfilesettingspage.cpp \
    abstracteditorsupport.cpp \
    cppfindreferences.cpp \
    cppcodeformatter.cpp \
    symbolsfindfilter.cpp \
    uicodecompletionsupport.cpp \
    insertionpointlocator.cpp \
    cpprefactoringchanges.cpp \
    cppcompletionassist.cpp \
    cppcodestylesettingspage.cpp \
    cpptoolssettings.cpp \
    cppcodestylesettings.cpp \
    cppcodestylesettingsfactory.cpp \
    cppcodestylepreferences.cpp \
    cpptoolsreuse.cpp

FORMS += completionsettingspage.ui \
    cppfilesettingspage.ui \
    cppcodestylesettingspage.ui
