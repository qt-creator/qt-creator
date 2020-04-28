add_qtc_library(clangpchmanagerbackend_lib STATIC
  DEPENDS clangrefactoringbackend_lib ClangSupport
  PUBLIC_DEPENDS libclang
  PUBLIC_DEFINES CLANGSUPPORT_BUILD_LIB
  PUBLIC_INCLUDES
    ../../clangrefactoringbackend/source
    ${CLANG_INCLUDE_DIRS}
  SOURCES
    builddependenciesprovider.cpp builddependenciesprovider.h
    builddependenciesproviderinterface.h
    builddependenciesstorage.h
    builddependenciesstorageinterface.h
    builddependency.h
    builddependencycollector.cpp builddependencycollector.h
    builddependencygeneratorinterface.h
    clangpchmanagerbackend_global.h
    collectbuilddependencyaction.h
    collectbuilddependencypreprocessorcallbacks.h
    collectbuilddependencytoolaction.h
    collectusedmacroactionfactory.h
    collectusedmacrosaction.h
    collectusedmacrosandsourcespreprocessorcallbacks.h
    generatepchactionfactory.h
    pchcreator.cpp pchcreator.h
    pchcreatorinterface.h
    pchmanagerserver.cpp pchmanagerserver.h
    pchnotcreatederror.h
    pchtask.h
    pchtaskgenerator.cpp pchtaskgenerator.h
    pchtaskgeneratorinterface.h
    pchtaskqueue.cpp
    pchtaskqueue.h
    pchtaskqueueinterface.h
    pchtasksmerger.cpp
    pchtasksmerger.h
    pchtasksmergerinterface.h
    precompiledheaderstorage.h
    precompiledheaderstorageinterface.h
    processorinterface.h
    processormanager.h
    processormanagerinterface.h
    projectpartsmanager.cpp projectpartsmanager.h
    projectpartsmanagerinterface.h
    queueinterface.h
    taskscheduler.h
    taskschedulerinterface.h
    toolchainargumentscache.h
    usedmacrofilter.h
    usedmacrosandsourcescollector.cpp usedmacrosandsourcescollector.h
)
