TARGET = CtfVisualizer
TEMPLATE = lib

QT += quick quickwidgets

include(../../qtcreatorplugin.pri)
include(ctfvisualizer_dependencies.pri)

DEFINES += CTFVISUALIZER_LIBRARY

# CtfVisualizer files

SOURCES += \
    ctfstatisticsmodel.cpp \
    ctfstatisticsview.cpp \
    ctfvisualizerplugin.cpp \
    ctfvisualizertool.cpp \
    ctftimelinemodel.cpp \
    ctftracemanager.cpp \
    ctfvisualizertraceview.cpp

HEADERS += \
    ctfstatisticsmodel.h \
    ctfstatisticsview.h \
    ctfvisualizerplugin.h \
    ctfvisualizertool.h\
    ctftimelinemodel.h \
    ctftracemanager.h \
    ctfvisualizerconstants.h \
    ctfvisualizertraceview.h \
    ../../libs/3rdparty/json/json.hpp

OTHER_FILES += \
    CtfVisualizer.json.in
