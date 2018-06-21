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
    $$PWD/storagesqlitestatementfactory.h \
    $$PWD/symbolindexing.h \
    $$PWD/symbolindexinginterface.h \
    $$PWD/collectmacrospreprocessorcallbacks.h \
    $$PWD/projectpartentry.h \
    $$PWD/symbolsvisitorbase.h \
    $$PWD/usedmacro.h \
    $$PWD/sourcedependency.h \
    $$PWD/filestatus.h \
    $$PWD/projectpartartefactexception.h \
    $$PWD/projectpartartefact.h \
    $$PWD/filestatuscache.h \
    $$PWD/indexdataconsumer.h

!isEmpty(LIBTOOLING_LIBS) {
SOURCES += \
    $$PWD/refactoringcompilationdatabase.cpp \
    $$PWD/symbolfinder.cpp \
    $$PWD/symbollocationfinderaction.cpp \
    $$PWD/refactoringserver.cpp \
    $$PWD/macropreprocessorcallbacks.cpp \
    $$PWD/findusrforcursoraction.cpp \
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
    $$PWD/symbolfinder.h \
    $$PWD/symbollocationfinderaction.h \
    $$PWD/refactoringserver.h \
    $$PWD/macropreprocessorcallbacks.h \
    $$PWD/sourcelocationsutils.h \
    $$PWD/findcursorusr.h \
    $$PWD/findusrforcursoraction.h \
    $$PWD/findlocationsofusrs.h \
    $$PWD/clangquery.h \
    $$PWD/clangtool.h \
    $$PWD/sourcerangeextractor.h \
    $$PWD/locationsourcefilecallbacks.h \
    $$PWD/collectsymbolsaction.h \
    $$PWD/collectmacrossourcefilecallbacks.h \
    $$PWD/symbolscollector.h \
    $$PWD/clangquerygatherer.h
}

SOURCES += \
    $$PWD/sourcerangefilter.cpp \
    $$PWD/symbolindexer.cpp \
    $$PWD/projectpartartefact.cpp \
    $$PWD/filestatuscache.cpp
