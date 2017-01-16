# Currently there are no tests for the project explorer plugin, but we include
# headers from it that needs to have the export/import adapted for Windows.
shared {
    DEFINES += CPPTOOLS_LIBRARY
    DEFINES += PROJECTEXPLORER_LIBRARY
} else {
    DEFINES += CPPTOOLS_STATIC_LIBRARY
    DEFINES += PROJECTEXPLORER_STATIC_LIBRARY
}

HEADERS += \
    $$PWD/cppprojectfile.h \
    $$PWD/senddocumenttracker.h \
    $$PWD/projectpart.h \
    $$PWD/compileroptionsbuilder.h \
    $$PWD/cppprojectfilecategorizer.h \
    $$PWD/clangcompileroptionsbuilder.h \
    $$PWD/cppbaseprojectpartbuilder.h \
    $$PWD/projectinfo.h \
    $$PWD/cppprojectinterface.h \
    $$PWD/cppprojectpartchooser.h \

SOURCES += \
    $$PWD/cppprojectfile.cpp \
    $$PWD/senddocumenttracker.cpp \
    $$PWD/projectpart.cpp \
    $$PWD/compileroptionsbuilder.cpp \
    $$PWD/cppprojectfilecategorizer.cpp \
    $$PWD/clangcompileroptionsbuilder.cpp \
    $$PWD/cppbaseprojectpartbuilder.cpp \
    $$PWD/projectinfo.cpp \
    $$PWD/cppprojectpartchooser.cpp \
