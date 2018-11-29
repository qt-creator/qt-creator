INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/builddependenciesprovider.cpp \
    $$PWD/pchmanagerserver.cpp \
    $$PWD/projectparts.cpp \
    $$PWD/projectpartqueue.cpp \
    $$PWD/pchtaskgenerator.cpp

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
    $$PWD/precompiledheaderstorageinterface.h \
    $$PWD/pchtaskgenerator.h \
    $$PWD/pchtask.h \
    $$PWD/builddependenciesproviderinterface.h \
    $$PWD/builddependenciesprovider.h \
    $$PWD/builddependenciesstorageinterface.h \
    $$PWD/builddependency.h \
    $$PWD/modifiedtimecheckerinterface.h \
    $$PWD/sourceentry.h \
    $$PWD/builddependenciesstorage.h \
    $$PWD/builddependencygeneratorinterface.h

!isEmpty(LIBTOOLING_LIBS) {
SOURCES += \
    $$PWD/usedmacrosandsourcescollector.cpp \
    $$PWD/pchcreator.cpp \
    $$PWD/builddependencycollector.cpp

HEADERS += \
    $$PWD/collectusedmacroactionfactory.h \
    $$PWD/collectusedmacrosaction.h \
    $$PWD/collectusedmacrosandsourcespreprocessorcallbacks.h \
    $$PWD/pchcreator.h \
    $$PWD/processormanager.h \
    $$PWD/usedmacrosandsourcescollector.h \
    $$PWD/builddependencycollector.h \
    $$PWD/collectbuilddependencytoolaction.h \
    $$PWD/collectbuilddependencyaction.h \
    $$PWD/collectbuilddependencypreprocessorcallbacks.h
}
