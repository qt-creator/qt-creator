INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/pchmanagerserver.cpp \
    $$PWD/projectparts.cpp \
    $$PWD/projectpartqueue.cpp \
    $$PWD/pchtaskgenerator.cpp \
    $$PWD/builddependenciesprovider.cpp

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
    $$PWD/usedmacroandsourcestorageinterface.h \
    $$PWD/usedmacroandsourcestorage.h \
    $$PWD/pchtaskgenerator.h \
    $$PWD/pchtask.h \
    $$PWD/builddependenciesproviderinterface.h \
    $$PWD/builddependenciesprovider.h \
    $$PWD/builddependenciesstorageinterface.h \
    $$PWD/builddependency.h \
    $$PWD/modifiedtimecheckerinterface.h \
    $$PWD/sourceentry.h \
    $$PWD/builddependenciesgeneratorinterface.h

!isEmpty(LIBTOOLING_LIBS) {
SOURCES += \
    $$PWD/usedmacrosandsourcescollector.cpp \
    $$PWD/includecollector.cpp \
    $$PWD/pchcreator.cpp

HEADERS += \
    $$PWD/includecollector.h \
    $$PWD/collectincludestoolaction.h \
    $$PWD/collectincludesaction.h \
    $$PWD/collectincludespreprocessorcallbacks.h \
    $$PWD/collectusedmacroactionfactory.h \
    $$PWD/collectusedmacrosaction.h \
    $$PWD/collectusedmacrosandsourcespreprocessorcallbacks.h \
    $$PWD/pchcreator.h \
    $$PWD/processormanager.h \
    $$PWD/usedmacrosandsourcescollector.h
}
