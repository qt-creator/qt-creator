add_qtc_library(clangbackend_lib STATIC
  DEPENDS libclang Sqlite ClangSupport
  PUBLIC_DEFINES CLANGSUPPORT_BUILD_LIB
  PUBLIC_INCLUDES
    ${CLANG_INCLUDE_DIRS}
    "${CMAKE_CURRENT_LIST_DIR}"
  SOURCES
    clangasyncjob.h
    clangbackend_global.h
    clangclock.h
    clangcodecompleteresults.cpp clangcodecompleteresults.h
    clangcodemodelserver.cpp clangcodemodelserver.h
    clangcompletecodejob.cpp clangcompletecodejob.h
    clangdocument.cpp clangdocument.h
    clangdocumentjob.h
    clangdocumentprocessor.cpp clangdocumentprocessor.h
    clangdocumentprocessors.cpp clangdocumentprocessors.h
    clangdocuments.cpp clangdocuments.h
    clangdocumentsuspenderresumer.cpp clangdocumentsuspenderresumer.h
    clangexceptions.cpp clangexceptions.h
    clangfilepath.cpp clangfilepath.h
    clangfilesystemwatcher.cpp clangfilesystemwatcher.h
    clangfollowsymbol.cpp clangfollowsymbol.h
    clangfollowsymboljob.cpp clangfollowsymboljob.h
    clangiasyncjob.cpp clangiasyncjob.h
    clangjobcontext.cpp clangjobcontext.h
    clangjobqueue.cpp clangjobqueue.h
    clangjobrequest.cpp clangjobrequest.h
    clangjobs.cpp clangjobs.h
    clangparsesupportivetranslationunitjob.cpp clangparsesupportivetranslationunitjob.h
    clangreferencescollector.cpp clangreferencescollector.h
    clangrequestannotationsjob.cpp clangrequestannotationsjob.h
    clangrequestreferencesjob.cpp clangrequestreferencesjob.h
    clangrequesttooltipjob.cpp clangrequesttooltipjob.h
    clangresumedocumentjob.cpp clangresumedocumentjob.h
    clangstring.h
    clangsupportivetranslationunitinitializer.cpp clangsupportivetranslationunitinitializer.h
    clangsuspenddocumentjob.cpp clangsuspenddocumentjob.h
    clangtooltipinfocollector.cpp clangtooltipinfocollector.h
    clangtranslationunit.cpp clangtranslationunit.h
    clangtranslationunits.cpp clangtranslationunits.h
    clangtranslationunitupdater.cpp clangtranslationunitupdater.h
    clangtype.cpp clangtype.h
    clangunsavedfilesshallowarguments.cpp clangunsavedfilesshallowarguments.h
    clangupdateannotationsjob.cpp clangupdateannotationsjob.h
    clangupdateextraannotationsjob.cpp clangupdateextraannotationsjob.h
    codecompleter.cpp codecompleter.h
    codecompletionchunkconverter.cpp codecompletionchunkconverter.h
    codecompletionsextractor.cpp codecompletionsextractor.h
    commandlinearguments.cpp commandlinearguments.h
    cursor.cpp cursor.h
    diagnostic.cpp diagnostic.h
    diagnosticset.cpp diagnosticset.h
    diagnosticsetiterator.h
    fixit.cpp fixit.h
    fulltokeninfo.cpp fulltokeninfo.h
    skippedsourceranges.cpp skippedsourceranges.h
    sourcelocation.cpp sourcelocation.h
    sourcerange.cpp sourcerange.h
    token.cpp token.h
    tokeninfo.cpp tokeninfo.h
    tokenprocessor.h
    tokenprocessoriterator.h
    unsavedfile.cpp unsavedfile.h
    unsavedfiles.cpp unsavedfiles.h
    utf8positionfromlinecolumn.cpp utf8positionfromlinecolumn.h
)
