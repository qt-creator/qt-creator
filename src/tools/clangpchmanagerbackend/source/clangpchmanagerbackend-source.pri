INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/builddependenciesprovider.cpp \
    $$PWD/pchmanagerserver.cpp \
    $$PWD/pchtaskgenerator.cpp \
    $$PWD/pchtasksmerger.cpp \
    $$PWD/pchtaskqueue.cpp \
    $$PWD/projectpartsmanager.cpp

HEADERS += \
    $$PWD/pchmanagerserver.h \
    $$PWD/clangpchmanagerbackend_global.h \
    $$PWD/pchnotcreatederror.h \
    $$PWD/environment.h \
    $$PWD/pchcreatorinterface.h \
    $$PWD/projectpartsmanager.h \
    $$PWD/projectpartsmanagerinterface.h \
    $$PWD/queueinterface.h \
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
    $$PWD/builddependencygeneratorinterface.h \
    $$PWD/usedmacrofilter.h \
    $$PWD/pchtasksmergerinterface.h \
    $$PWD/pchtasksmerger.h \
    $$PWD/pchtaskqueueinterface.h \
    $$PWD/pchtaskqueue.h \
    $$PWD/generatepchactionfactory.h \
    $$PWD/pchtaskgeneratorinterface.h \
    $$PWD/toolchainargumentscache.h \
    $$PWD/modifiedtimechecker.h

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
