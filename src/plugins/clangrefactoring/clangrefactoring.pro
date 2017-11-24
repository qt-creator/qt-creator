include(../../qtcreatorplugin.pri)
include(clangrefactoring-source.pri)
include(../../shared/clang/clang_installation.pri)

include(../../shared/clang/clang_defines.pri)

requires(!isEmpty(LIBTOOLING_LIBS))

HEADERS += \
    clangrefactoringplugin.h \
    baseclangquerytexteditorwidget.h \
    clangqueryexampletexteditorwidget.h \
    clangqueryhoverhandler.h \
    clangqueryprojectsfindfilterwidget.h \
    clangquerytexteditorwidget.h \
    qtcreatorclangqueryfindfilter.h \
    qtcreatorclassesfilter.h \
    qtcreatorfunctionsfilter.h \
    qtcreatorincludesfilter.h \
    qtcreatorlocatorfilter.h \
    qtcreatorsearch.h \
    qtcreatorsearchhandle.h \
    qtcreatorsymbolsfindfilter.h \
    querysqlitestatementfactory.h \
    sourcelocations.h \
    symbolsfindfilterconfigwidget.h \
    symbolquery.h

SOURCES += \
    clangrefactoringplugin.cpp \
    baseclangquerytexteditorwidget.cpp \
    clangqueryexampletexteditorwidget.cpp \
    clangqueryhoverhandler.cpp \
    clangqueryprojectsfindfilterwidget.cpp \
    clangquerytexteditorwidget.cpp \
    qtcreatorclangqueryfindfilter.cpp \
    qtcreatorclassesfilter.cpp \
    qtcreatorincludesfilter.cpp \
    qtcreatorfunctionsfilter.cpp \
    qtcreatorlocatorfilter.cpp \
    qtcreatorsearch.cpp \
    qtcreatorsearchhandle.cpp \
    qtcreatorsymbolsfindfilter.cpp \
    symbolsfindfilterconfigwidget.cpp \
    symbolquery.cpp

FORMS += \
    clangqueryprojectsfindfilter.ui
