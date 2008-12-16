TEMPLATE = lib
TARGET = CppTools
include(../../qworkbenchplugin.pri)
include(../../plugins/quickopen/quickopen.pri)
include(cpptools_dependencies.pri)

# DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
INCLUDEPATH += .
DEFINES += CPPTOOLS_LIBRARY
HEADERS += cpptools_global.h \
    cppquickopenfilter.h \
    cppclassesfilter.h \
    searchsymbols.h \
    cppfunctionsfilter.h \
    completionsettingspage.h
SOURCES += cppquickopenfilter.cpp \
    cpptoolseditorsupport.cpp \
    cppclassesfilter.cpp \
    searchsymbols.cpp \
    cppfunctionsfilter.cpp \
    completionsettingspage.cpp

# Input
SOURCES += cpptoolsplugin.cpp \
    cppmodelmanager.cpp \
    cppcodecompletion.cpp
HEADERS += cpptoolsplugin.h \
    cppmodelmanager.h \
    cppcodecompletion.h \
    cppmodelmanagerinterface.h \
    cpptoolseditorsupport.h \
    cpptoolsconstants.h
FORMS += completionsettingspage.ui
