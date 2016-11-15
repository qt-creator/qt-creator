include(../../qtcreatorplugin.pri)
include(clangrefactoring-source.pri)
include(../../shared/clang/clang_installation.pri)

include(../../shared/clang/clang_defines.pri)

HEADERS += \
    $$PWD/clangrefactoringplugin.h \
    qtcreatorsearch.h \
    qtcreatorsearchhandle.h \
    qtcreatorclangqueryfindfilter.h

SOURCES += \
    $$PWD/clangrefactoringplugin.cpp \
    qtcreatorsearch.cpp \
    qtcreatorsearchhandle.cpp \
    qtcreatorclangqueryfindfilter.cpp
