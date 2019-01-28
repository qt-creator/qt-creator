include(../../qtcreatorplugin.pri)
include(clangformat-source.pri)
include(../../shared/clang/clang_installation.pri)

include(../../shared/clang/clang_defines.pri)

requires(!isEmpty(CLANGFORMAT_LIBS))

win32 {
    LLVM_BUILDMODE = $$system($$llvm_config --build-mode, lines)
    CONFIG(debug, debug|release):requires(equals(LLVM_BUILDMODE, "Debug"))
}

LIBS += $$CLANGFORMAT_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH

QMAKE_CXXFLAGS_WARN_ON *= $$LLVM_CXXFLAGS_WARNINGS
QMAKE_CXXFLAGS *= $$LLVM_CXXFLAGS
# clang/Format/Format.h has intentional multiline comments
gcc:QMAKE_CXXFLAGS *= -Wno-comment
unix:!macos:QMAKE_LFLAGS += -Wl,--exclude-libs,ALL

SOURCES += \
    clangformatconfigwidget.cpp \
    clangformatindenter.cpp \
    clangformatplugin.cpp \
    clangformatsettings.cpp \
    clangformatutils.cpp

HEADERS += \
    clangformatconfigwidget.h \
    clangformatindenter.h \
    clangformatplugin.h \
    clangformatsettings.h \
    clangformatutils.h

FORMS += \
    clangformatconfigwidget.ui
