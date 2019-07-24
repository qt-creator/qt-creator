INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/clangrefactoringbackend_global.h \
    $$PWD/sourcerangefilter.h \
    $$PWD/symbolindexer.h \
    $$PWD/symbolentry.h \
    $$PWD/sourcelocationentry.h \
    $$PWD/symbolscollectorinterface.h \
    $$PWD/symbolstorageinterface.h \
    $$PWD/symbolstorage.h \
    $$PWD/symbolindexing.h \
    $$PWD/symbolindexinginterface.h \
    $$PWD/collectmacrospreprocessorcallbacks.h \
    $$PWD/projectpartentry.h \
    $$PWD/usedmacro.h \
    $$PWD/sourcedependency.h \
    $$PWD/sourcesmanager.h \
    $$PWD/symbolindexertaskqueue.h \
    $$PWD/symbolindexertaskqueueinterface.h \
    $$PWD/symbolindexertask.h

!isEmpty(LIBTOOLING_LIBS) {
SOURCES += \
    $$PWD/refactoringcompilationdatabase.cpp \
    $$PWD/refactoringserver.cpp \
    $$PWD/macropreprocessorcallbacks.cpp \
    $$PWD/clangquery.cpp \
    $$PWD/clangtool.cpp \
    $$PWD/sourcerangeextractor.cpp \
    $$PWD/locationsourcefilecallbacks.cpp \
    $$PWD/collectsymbolsaction.cpp \
    $$PWD/collectmacrossourcefilecallbacks.cpp \
    $$PWD/symbolscollector.cpp \
    $$PWD/clangquerygatherer.cpp \
    $$PWD/symbolindexing.cpp \
    $$PWD/indexdataconsumer.cpp

HEADERS += \
    $$PWD/refactoringcompilationdatabase.h \
    $$PWD/refactoringserver.h \
    $$PWD/macropreprocessorcallbacks.h \
    $$PWD/sourcelocationsutils.h \
    $$PWD/clangquery.h \
    $$PWD/clangtool.h \
    $$PWD/sourcerangeextractor.h \
    $$PWD/locationsourcefilecallbacks.h \
    $$PWD/collectsymbolsaction.h \
    $$PWD/collectmacrossourcefilecallbacks.h \
    $$PWD/symbolscollector.h \
    $$PWD/symbolsvisitorbase.h \
    $$PWD/indexdataconsumer.h \
    $$PWD/clangquerygatherer.h
}

SOURCES += \
    $$PWD/sourcerangefilter.cpp \
    $$PWD/symbolindexer.cpp
