INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/pchmanagerserver.cpp \
    $$PWD/projectparts.cpp \
    $$PWD/projectpartqueue.cpp

HEADERS += \
    $$PWD/pchmanagerserver.h \
    $$PWD/clangpchmanagerbackend_global.h \
    $$PWD/pchnotcreatederror.h \
    $$PWD/environment.h \
    $$PWD/projectparts.h \
    $$PWD/pchcreatorinterface.h \
    $$PWD/projectpartsinterface.h \
    $$PWD/projectpartqueue.h \
    $$PWD/queueinterface.h \
    $$PWD/projectpartqueueinterface.h \
    $$PWD/processormanagerinterface.h \
    $$PWD/processorinterface.h \
    $$PWD/taskscheduler.h \
    $$PWD/taskschedulerinterface.h \
    $$PWD/precompiledheaderstorage.h \
    $$PWD/precompiledheaderstorageinterface.h

!isEmpty(LIBTOOLING_LIBS) {
SOURCES += \
    $$PWD/includecollector.cpp \
    $$PWD/pchcreator.cpp

HEADERS += \
    $$PWD/includecollector.h \
    $$PWD/collectincludestoolaction.h \
    $$PWD/collectincludesaction.h \
    $$PWD/collectincludespreprocessorcallbacks.h \
    $$PWD/pchcreator.h \
    $$PWD/processormanager.h
}
