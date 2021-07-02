shared {
    DEFINES += CPPTOOLS_LIBRARY
} else {
    DEFINES += CPPTOOLS_STATIC_LIBRARY
}

HEADERS += \
    $$PWD/cppprojectfile.h \
    $$PWD/senddocumenttracker.h \
    $$PWD/projectpart.h \
    $$PWD/cppprojectfilecategorizer.h \
    $$PWD/projectinfo.h \
    $$PWD/cppprojectinfogenerator.cpp \
    $$PWD/headerpathfilter.h

SOURCES += \
    $$PWD/cppprojectfile.cpp \
    $$PWD/senddocumenttracker.cpp \
    $$PWD/projectpart.cpp \
    $$PWD/cppprojectfilecategorizer.cpp \
    $$PWD/projectinfo.cpp \
    $$PWD/cppprojectinfogenerator.cpp \
    $$PWD/headerpathfilter.cpp
