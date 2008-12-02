TEMPLATE = lib
TARGET = CppTools
include(../../qworkbenchplugin.pri)
include(../../plugins/quickopen/quickopen.pri)
include(cpptools_dependencies.pri)

#DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
unix:QMAKE_CXXFLAGS_DEBUG+=-O3

INCLUDEPATH += .

DEFINES += CPPTOOLS_LIBRARY

CONFIG += help
include(rpp/rpp.pri)|error("Can't find RPP")

HEADERS += \
    cpptools_global.h \
    cppquickopenfilter.h

SOURCES += \
    cppquickopenfilter.cpp \
    cpptoolseditorsupport.cpp

# Input
SOURCES += cpptools.cpp \
    cppmodelmanager.cpp \
    cppcodecompletion.cpp \
    cpphoverhandler.cpp

HEADERS += cpptools.h \
    cppmodelmanager.h \
    cppcodecompletion.h \
    cpphoverhandler.h \
    cppmodelmanagerinterface.h \
    cpptoolseditorsupport.h \
    cpptoolsconstants.h

RESOURCES += cpptools.qrc
