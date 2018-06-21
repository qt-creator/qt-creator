INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/pchmanagerserver.cpp \
    $$PWD/projectparts.cpp

HEADERS += \
    $$PWD/pchmanagerserver.h \
    $$PWD/clangpchmanagerbackend_global.h \
    $$PWD/pchnotcreatederror.h \
    $$PWD/environment.h \
    $$PWD/projectparts.h \
    $$PWD/pchcreatorinterface.h \
    $$PWD/projectpartsinterface.h \
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
