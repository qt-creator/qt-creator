INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/pchmanagerserver.cpp \
    $$PWD/clangpathwatcher.cpp \
    $$PWD/projectparts.cpp \
    $$PWD/idpaths.cpp \
    $$PWD/pchcreatorinterface.cpp \
    $$PWD/clangpathwatcherinterface.cpp \
    $$PWD/projectpartsinterface.cpp \
    $$PWD/clangpathwatchernotifier.cpp \
    $$PWD/pchgeneratornotifierinterface.cpp \
    $$PWD/pchgeneratorinterface.cpp

HEADERS += \
    $$PWD/clangpchmanagerbackend_global.h \
    $$PWD/pchnotcreatederror.h \
    $$PWD/environment.h \
    $$PWD/clangpathwatcher.h \
    $$PWD/projectparts.h \
    $$PWD/stringcache.h \
    $$PWD/idpaths.h \
    $$PWD/pchcreatorinterface.h \
    $$PWD/clangpathwatcherinterface.h \
    $$PWD/projectpartsinterface.h \
    $$PWD/clangpathwatchernotifier.h \
    $$PWD/changedfilepathcompressor.h \
    $$PWD/pchgenerator.h \
    $$PWD/pchgeneratornotifierinterface.h \
    $$PWD/pchgeneratorinterface.h

!isEmpty(LIBTOOLING_LIBS) {
SOURCES += \
    $$PWD/includecollector.cpp \
    $$PWD/pchcreator.cpp

HEADERS += \
    $$PWD/includecollector.h \
    $$PWD/collectincludestoolaction.h \
    $$PWD/collectincludesaction.h \
    $$PWD/collectincludespreprocessorcallbacks.h \
    $$PWD/pchcreator.h
}
