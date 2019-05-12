add_qtc_executable(clangbackend
  DEPENDS ClangSupport libclang Sqlite Qt5::Core Qt5::Network
  INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/source" ${CLANG_INCLUDE_DIRS}
  SOURCES
    ../qtcreatorcrashhandler/crashhandlersetup.cpp ../qtcreatorcrashhandler/crashhandlersetup.h

    clangbackendmain.cpp
    source/clangasyncjob.h
    source/clangbackend_global.h
    source/clangclock.h
    source/clangcodecompleteresults.cpp source/clangcodecompleteresults.h
    source/clangcodemodelserver.cpp source/clangcodemodelserver.h
    source/clangcompletecodejob.cpp source/clangcompletecodejob.h
    source/clangdocument.cpp source/clangdocument.h
    source/clangdocumentjob.h
    source/clangdocumentprocessor.cpp source/clangdocumentprocessor.h
    source/clangdocumentprocessors.cpp source/clangdocumentprocessors.h
    source/clangdocuments.cpp source/clangdocuments.h
    source/clangdocumentsuspenderresumer.cpp source/clangdocumentsuspenderresumer.h
    source/clangexceptions.cpp source/clangexceptions.h
    source/clangfilepath.cpp source/clangfilepath.h
    source/clangfilesystemwatcher.cpp source/clangfilesystemwatcher.h
    source/clangfollowsymbol.cpp source/clangfollowsymbol.h
    source/clangfollowsymboljob.cpp source/clangfollowsymboljob.h
    source/clangiasyncjob.cpp source/clangiasyncjob.h
    source/clangjobcontext.cpp source/clangjobcontext.h
    source/clangjobqueue.cpp source/clangjobqueue.h
    source/clangjobrequest.cpp source/clangjobrequest.h
    source/clangjobs.cpp source/clangjobs.h
    source/clangparsesupportivetranslationunitjob.cpp source/clangparsesupportivetranslationunitjob.h
    source/clangreferencescollector.cpp source/clangreferencescollector.h
    source/clangrequestannotationsjob.cpp source/clangrequestannotationsjob.h
    source/clangrequestreferencesjob.cpp source/clangrequestreferencesjob.h
    source/clangrequesttooltipjob.cpp source/clangrequesttooltipjob.h
    source/clangresumedocumentjob.cpp source/clangresumedocumentjob.h
    source/clangstring.h
    source/clangsupportivetranslationunitinitializer.cpp source/clangsupportivetranslationunitinitializer.h
    source/clangsuspenddocumentjob.cpp source/clangsuspenddocumentjob.h
    source/clangtooltipinfocollector.cpp source/clangtooltipinfocollector.h
    source/clangtranslationunit.cpp source/clangtranslationunit.h
    source/clangtranslationunits.cpp source/clangtranslationunits.h
    source/clangtranslationunitupdater.cpp source/clangtranslationunitupdater.h
    source/clangtype.cpp source/clangtype.h
    source/clangunsavedfilesshallowarguments.cpp source/clangunsavedfilesshallowarguments.h
    source/clangupdateannotationsjob.cpp source/clangupdateannotationsjob.h
    source/clangupdateextraannotationsjob.cpp source/clangupdateextraannotationsjob.h
    source/codecompleter.cpp source/codecompleter.h
    source/codecompletionchunkconverter.cpp source/codecompletionchunkconverter.h
    source/codecompletionsextractor.cpp source/codecompletionsextractor.h
    source/commandlinearguments.cpp source/commandlinearguments.h
    source/cursor.cpp source/cursor.h
    source/diagnostic.cpp source/diagnostic.h
    source/diagnosticset.cpp source/diagnosticset.h
    source/diagnosticsetiterator.h
    source/fixit.cpp source/fixit.h
    source/fulltokeninfo.cpp source/fulltokeninfo.h
    source/skippedsourceranges.cpp source/skippedsourceranges.h
    source/sourcelocation.cpp source/sourcelocation.h
    source/sourcerange.cpp source/sourcerange.h
    source/token.cpp source/token.h
    source/tokeninfo.cpp source/tokeninfo.h
    source/tokenprocessor.h
    source/tokenprocessoriterator.h
    source/unsavedfile.cpp source/unsavedfile.h
    source/unsavedfiles.cpp source/unsavedfiles.h
    source/utf8positionfromlinecolumn.cpp source/utf8positionfromlinecolumn.h
)
