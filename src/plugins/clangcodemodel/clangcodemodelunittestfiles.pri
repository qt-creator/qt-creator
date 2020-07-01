INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/clangcompletionchunkstotextconverter.cpp \
    $$PWD/clangdiagnosticfilter.cpp \
    $$PWD/clangfixitoperation.cpp \
    $$PWD/clanghighlightingresultreporter.cpp \
    $$PWD/clanguiheaderondiskmanager.cpp

!isEmpty(QTC_UNITTEST_BUILD_CPP_PARSER):SOURCES+= \
    $$PWD/clangactivationsequenceprocessor.cpp \
    $$PWD/clangactivationsequencecontextprocessor.cpp

HEADERS += \
    $$PWD/clangcompletionchunkstotextconverter.h \
    $$PWD/clangdiagnosticfilter.h \
    $$PWD/clangfixitoperation.h \
    $$PWD/clanghighlightingresultreporter.h \
    $$PWD/clangisdiagnosticrelatedtolocation.h \
    $$PWD/clanguiheaderondiskmanager.h

!isEmpty(QTC_UNITTEST_BUILD_CPP_PARSER):SOURCES+= \
    $$PWD/clangactivationsequencecontextprocessor.h \
    $$PWD/clangactivationsequenceprocessor.h \
    $$PWD/clangcompletioncontextanalyzer.cpp \
    $$PWD/clangcompletioncontextanalyzer.h


