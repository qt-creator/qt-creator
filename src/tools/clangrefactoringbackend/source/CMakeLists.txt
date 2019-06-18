add_qtc_library(clangrefactoringbackend_lib STATIC
  PUBLIC_DEPENDS
    Threads::Threads
    LLVMCore libclang
    clangAST clangASTMatchers clangBasic clangDynamicASTMatchers clangFrontend
    clangHandleCXX clangIndex clangLex
    clangSerialization clangTooling clangQuery
    ClangSupport
  PUBLIC_DEFINES CLANGSUPPORT_BUILD_LIB
  PUBLIC_INCLUDES
    ${CLANG_INCLUDE_DIRS}
    "../../clangpchmanagerbackend/source"
  SOURCES
    clangquery.cpp clangquery.h
    clangquerygatherer.cpp clangquerygatherer.h
    clangrefactoringbackend_global.h
    clangtool.cpp clangtool.h
    collectmacrospreprocessorcallbacks.h
    collectmacrossourcefilecallbacks.cpp collectmacrossourcefilecallbacks.h
    collectsymbolsaction.cpp collectsymbolsaction.h
    filestatus.h
    filestatuscache.cpp filestatuscache.h
    filestatuspreprocessorcallbacks.cpp filestatuspreprocessorcallbacks.h
    findcursorusr.h
    findlocationsofusrs.h
    findusrforcursoraction.cpp findusrforcursoraction.h
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
    symbolfinder.cpp symbolfinder.h
    symbolindexer.cpp symbolindexer.h
    symbolindexertask.h
    symbolindexertaskqueue.h
    symbolindexertaskqueueinterface.h
    symbolindexing.cpp symbolindexing.h
    symbolindexinginterface.h
    symbollocationfinderaction.cpp symbollocationfinderaction.h
    symbolscollector.cpp symbolscollector.h
    symbolscollectorinterface.h
    symbolstorage.h
    symbolstorageinterface.h
    symbolsvisitorbase.h
    usedmacro.h
)
