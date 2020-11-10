add_qtc_library(clangpchmanagerbackend_lib STATIC
  CONDITION TARGET clangrefactoringbackend_lib
  DEPENDS clangrefactoringbackend_lib
  PUBLIC_DEPENDS libclang Qt5::Widgets # FIXME: change the way to get the gui pch to linkto
  DEFINES
    $<TARGET_PROPERTY:Utils,INTERFACE_COMPILE_DEFINITIONS>
  INCLUDES
    $<TARGET_PROPERTY:ClangSupport,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Utils,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC_INCLUDES
    ../../clangrefactoringbackend/source
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
