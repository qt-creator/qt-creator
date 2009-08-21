DEFINES += DEBUG_TRK=0
INCLUDEPATH *= $$PWD
SOURCES += $$PWD/launcher.cpp \
    $$PWD/trkutils.cpp \
    $$PWD/trkdevice.cpp
HEADERS += $$PWD/trkutils.h \
    $$PWD/trkfunctor.h \
    $$PWD/trkdevice.h \
    $$PWD/launcher.h
