INCLUDEPATH *= $$PWD

SOURCES += $$PWD/registryaccess.cpp
HEADERS += $$PWD/registryaccess.h

LIBS *= -lpsapi
# PS API and registry functions
contains(QMAKE_CXX, cl) {
    LIBS *= -ladvapi32
}
