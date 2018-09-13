include(../../qtcreatorplugin.pri)
include(../../shared/clang/clang_installation.pri)

include(../../shared/clang/clang_defines.pri)

requires(!isEmpty(LLVM_VERSION))

win32 {
    LLVM_BUILDMODE = $$system($$llvm_config --build-mode, lines)
    CONFIG(debug, debug|release):requires(equals(LLVM_BUILDMODE, "Debug"))
}

LIBS += $$CLANGFORMAT_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH

QMAKE_CXXFLAGS += $$LLVM_CXXFLAGS

SOURCES = \
    clangformatconfigwidget.cpp \
    clangformatindenter.cpp \
    clangformatplugin.cpp

HEADERS = \
    clangformatconfigwidget.h \
    clangformatindenter.h \
    clangformatplugin.h \
    clangformatconstants.h

FORMS += \
    clangformatconfigwidget.ui
