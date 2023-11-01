
function(extend_valgrind_test targetName)
  extend_qtc_test(${targetName}
    SOURCES_PREFIX "${PROJECT_SOURCE_DIR}/src/plugins/valgrind/"
    SOURCES
      callgrind/callgrindcallmodel.h callgrind/callgrindcallmodel.cpp
      callgrind/callgrindcostitem.h callgrind/callgrindcostitem.cpp
      callgrind/callgrindcycledetection.h callgrind/callgrindcycledetection.cpp
      callgrind/callgrinddatamodel.h callgrind/callgrinddatamodel.cpp
      callgrind/callgrindfunction.h callgrind/callgrindfunction_p.h callgrind/callgrindfunction.cpp
      callgrind/callgrindfunctioncall.h callgrind/callgrindfunctioncall.cpp
      callgrind/callgrindfunctioncycle.h callgrind/callgrindfunctioncycle.cpp
      callgrind/callgrindparsedata.h  callgrind/callgrindparsedata.cpp
      callgrind/callgrindparser.h callgrind/callgrindparser.cpp
      callgrind/callgrindproxymodel.h callgrind/callgrindproxymodel.cpp
      callgrind/callgrindstackbrowser.h callgrind/callgrindstackbrowser.cpp
      valgrindprocess.h valgrindprocess.cpp
      xmlprotocol/announcethread.h xmlprotocol/announcethread.cpp
      xmlprotocol/error.h xmlprotocol/error.cpp
      xmlprotocol/errorlistmodel.h xmlprotocol/errorlistmodel.cpp
      xmlprotocol/frame.h xmlprotocol/frame.cpp
      xmlprotocol/parser.h xmlprotocol/parser.cpp
      xmlprotocol/stack.h xmlprotocol/stack.cpp
      xmlprotocol/stackmodel.h xmlprotocol/stackmodel.cpp
      xmlprotocol/status.h xmlprotocol/status.cpp
      xmlprotocol/suppression.h xmlprotocol/suppression.cpp
  )
endfunction()
