include(../../qtcreatorplugin.pri)
include(clangrefactoring-source.pri)
include(../../shared/clang/clang_installation.pri)

include(../../shared/clang/clang_defines.pri)

requires(!isEmpty(LIBTOOLING_LIBS))

win32 {
    LLVM_BUILDMODE = $$system($$llvm_config --build-mode, lines)
    CONFIG(debug, debug|release):requires(equals(LLVM_BUILDMODE, "Debug"))
}

HEADERS += \
    clangrefactoringplugin.h \
    baseclangquerytexteditorwidget.h \
    clangqueryexampletexteditorwidget.h \
    clangqueryhoverhandler.h \
    clangqueryprojectsfindfilterwidget.h \
    clangquerytexteditorwidget.h \
    qtcreatorclangqueryfindfilter.h \
    qtcreatorsearch.h \
    qtcreatorsearchhandle.h \
    qtcreatorsymbolsfindfilter.h \
    querysqlitestatementfactory.h \
    sourcelocations.h \
    symbolsfindfilterconfigwidget.h \
    symbolquery.h \
    qtcreatoreditormanager.h \
    qtcreatorrefactoringprojectupdater.h

SOURCES += \
    clangrefactoringplugin.cpp \
    baseclangquerytexteditorwidget.cpp \
    clangqueryexampletexteditorwidget.cpp \
    clangqueryhoverhandler.cpp \
    clangqueryprojectsfindfilterwidget.cpp \
    clangquerytexteditorwidget.cpp \
    qtcreatorclangqueryfindfilter.cpp \
    qtcreatorsearch.cpp \
    qtcreatorsearchhandle.cpp \
    qtcreatorsymbolsfindfilter.cpp \
    symbolsfindfilterconfigwidget.cpp \
    qtcreatoreditormanager.cpp \
    qtcreatorrefactoringprojectupdater.cpp

FORMS += \
    clangqueryprojectsfindfilter.ui
