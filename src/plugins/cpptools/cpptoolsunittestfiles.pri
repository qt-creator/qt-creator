shared {
    DEFINES += CPPTOOLS_LIBRARY
} else {
    DEFINES += CPPTOOLS_STATIC_LIBRARY
}

HEADERS += \
    $$PWD/cppprojectfile.h \
    $$PWD/senddocumenttracker.h \
    $$PWD/projectpart.h \
    $$PWD/compileroptionsbuilder.h \
    $$PWD/cppprojectfilecategorizer.h \
    $$PWD/projectinfo.h \
    $$PWD/cppprojectinfogenerator.cpp \
    $$PWD/cppprojectpartchooser.h \

SOURCES += \
    $$PWD/cppprojectfile.cpp \
    $$PWD/senddocumenttracker.cpp \
    $$PWD/projectpart.cpp \
    $$PWD/compileroptionsbuilder.cpp \
    $$PWD/cppprojectfilecategorizer.cpp \
    $$PWD/projectinfo.cpp \
    $$PWD/cppprojectinfogenerator.cpp \
    $$PWD/cppprojectpartchooser.cpp \
