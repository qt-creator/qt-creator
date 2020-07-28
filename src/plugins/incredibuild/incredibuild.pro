TARGET = IncrediBuild
TEMPLATE = lib

include(../../qtcreatorplugin.pri)

DEFINES += INCREDIBUILD_LIBRARY

# IncrediBuild files

SOURCES += incredibuildplugin.cpp \
    buildconsolebuildstep.cpp \
    buildconsolestepfactory.cpp \
    buildconsolestepconfigwidget.cpp \
    commandbuilder.cpp \
    makecommandbuilder.cpp \
    cmakecommandbuilder.cpp \
    ibconsolebuildstep.cpp

HEADERS += incredibuildplugin.h \
    buildconsolestepconfigwidget.h \
    buildconsolestepfactory.h \
    cmakecommandbuilder.h \
    commandbuilder.h \
    incredibuild_global.h \
    incredibuildconstants.h \
    buildconsolebuildstep.h \
    makecommandbuilder.h \
    ibconsolebuildstep.h \

FORMS += \
    buildconsolebuildstep.ui
