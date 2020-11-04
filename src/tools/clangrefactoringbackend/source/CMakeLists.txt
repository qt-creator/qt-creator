add_qtc_library(clangrefactoringbackend_lib STATIC
  CONDITION WITH_TESTS OR TARGET ClangRefactoring OR TARGET ClangPchManager
  PUBLIC_DEPENDS
    Threads::Threads
    ${CLANG_TOOLING_LIBS}
    Qt5::Widgets # FIXME: change the way to get the gui pch to linkto
  INCLUDES
    $<TARGET_PROPERTY:ClangSupport,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC_INCLUDES
    ${CLANG_INCLUDE_DIRS}
    ../../clangpchmanagerbackend/source
    "${CMAKE_CURRENT_LIST_DIR}"
  SOURCES
    clangquery.cpp clangquery.h
    clangquerygatherer.cpp clangquerygatherer.h
    clangrefactoringbackend_global.h
    clangtool.cpp clangtool.h
    collectmacrospreprocessorcallbacks.h
    collectmacrossourcefilecallbacks.cpp collectmacrossourcefilecallbacks.h
    collectsymbolsaction.cpp collectsymbolsaction.h
    indexdataconsumer.cpp indexdataconsumer.h
    locationsourcefilecallbacks.cpp locationsourcefilecallbacks.h
    macropreprocessorcallbacks.cpp macropreprocessorcallbacks.h
    projectpartentry.h
    refactoringcompilationdatabase.cpp refactoringcompilationdatabase.h
    refactoringserver.cpp refactoringserver.h
    sourcedependency.h
    sourcelocationentry.h
    sourcelocationsutils.h
    sourcerangeextractor.cpp sourcerangeextractor.h
    sourcerangefilter.cpp sourcerangefilter.h
    sourcesmanager.h
    symbolentry.h
    symbolindexer.cpp symbolindexer.h
    symbolindexertask.h
    symbolindexertaskqueue.h
    symbolindexertaskqueueinterface.h
    symbolindexing.cpp symbolindexing.h
    symbolindexinginterface.h
    symbolscollector.cpp symbolscollector.h
    symbolscollectorinterface.h
    symbolstorage.h
    symbolstorageinterface.h
    symbolsvisitorbase.h
    usedmacro.h
)
