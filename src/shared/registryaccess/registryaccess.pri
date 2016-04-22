INCLUDEPATH *= $$PWD

SOURCES += $$PWD/registryaccess.cpp
HEADERS += $$PWD/registryaccess.h

LIBS *= -lpsapi
# PS API and registry functions
msvc {
    LIBS *= -ladvapi32
}
