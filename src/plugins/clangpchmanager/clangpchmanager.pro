include(../../qtcreatorplugin.pri)
include(clangpchmanager-source.pri)
include(../../shared/clang/clang_installation.pri)
include(../../shared/clang/clang_defines.pri)

requires(!isEmpty(LIBTOOLING_LIBS))

win32 {
    LLVM_BUILDMODE = $$system($$llvm_config --build-mode, lines)
    CONFIG(debug, debug|release):requires(equals(LLVM_BUILDMODE, "Debug"))
}

HEADERS += \
    $$PWD/clangpchmanagerplugin.h \
    clangindexingprojectsettingswidget.h \
    preprocessormacrowidget.h \
    qtcreatorprojectupdater.h

SOURCES += \
    $$PWD/clangpchmanagerplugin.cpp \
    clangindexingprojectsettingswidget.cpp \
    preprocessormacrowidget.cpp \
    qtcreatorprojectupdater.cpp

FORMS += \
    clangindexingprojectsettingswidget.ui
