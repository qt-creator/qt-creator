TEMPLATE = lib
TARGET = CppTools
include(../../qworkbenchplugin.pri)
include(../../plugins/quickopen/quickopen.pri)
include(cpptools_dependencies.pri)

# DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
INCLUDEPATH += .
DEFINES += CPPTOOLS_LIBRARY
CONFIG += help
HEADERS += cpptools_global.h \
    cppquickopenfilter.h \
    cppclassesfilter.h \
    searchsymbols.h \
    cppfunctionsfilter.h
SOURCES += cppquickopenfilter.cpp \
    cpptoolseditorsupport.cpp \
    cppclassesfilter.cpp \
    searchsymbols.cpp \
    cppfunctionsfilter.cpp

# Input
SOURCES += cpptoolsplugin.cpp \
    cppmodelmanager.cpp \
    cppcodecompletion.cpp \
    cpphoverhandler.cpp
HEADERS += cpptoolsplugin.h \
    cppmodelmanager.h \
    cppcodecompletion.h \
    cpphoverhandler.h \
    cppmodelmanagerinterface.h \
    cpptoolseditorsupport.h \
    cpptoolsconstants.h
RESOURCES += cpptools.qrc
