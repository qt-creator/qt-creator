set(CLANG_VERSION ${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH})

add_qtc_library(ClangSupport
  DEPENDS Threads::Threads
  PUBLIC_DEPENDS Utils Qt5::Core Qt5::Network
  PUBLIC_DEFINES
    CLANG_VERSION="${CLANG_VERSION}"
    CLANG_INCLUDE_DIR="${LLVM_LIBRARY_DIR}/clang/${CLANG_VERSION}/include"
    CLANG_BINDIR="${LLVM_TOOLS_BINARY_DIR}"
  DEFINES CLANGSUPPORT_BUILD_LIB
  PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}"
  SOURCES
    alivemessage.cpp alivemessage.h
    annotationsmessage.cpp annotationsmessage.h
    baseserverproxy.cpp baseserverproxy.h
    clangcodemodelclientinterface.cpp clangcodemodelclientinterface.h
    clangcodemodelclientmessages.h
    clangcodemodelclientproxy.cpp clangcodemodelclientproxy.h
    clangcodemodelconnectionclient.cpp clangcodemodelconnectionclient.h
    clangcodemodelserverinterface.cpp clangcodemodelserverinterface.h
    clangcodemodelservermessages.h
    clangcodemodelserverproxy.cpp clangcodemodelserverproxy.h
    clangsupport_global.h
    clangsupportdebugutils.cpp clangsupportdebugutils.h
    codecompletion.cpp codecompletion.h
    codecompletionchunk.cpp codecompletionchunk.h
    completionsmessage.cpp completionsmessage.h
    connectionclient.cpp connectionclient.h
    connectionserver.cpp connectionserver.h
    diagnosticcontainer.cpp diagnosticcontainer.h
    documentschangedmessage.cpp documentschangedmessage.h
    documentsclosedmessage.cpp documentsclosedmessage.h
    documentsopenedmessage.cpp documentsopenedmessage.h
    documentvisibilitychangedmessage.cpp documentvisibilitychangedmessage.h
    echomessage.cpp echomessage.h
    endmessage.cpp endmessage.h
    filecontainer.cpp filecontainer.h
    fixitcontainer.cpp fixitcontainer.h
    followsymbolmessage.cpp followsymbolmessage.h
    ipcclientinterface.h
    ipcclientprovider.h
    ipcinterface.h
    ipcserverinterface.h
    lineprefixer.cpp lineprefixer.h
    messageenvelop.cpp messageenvelop.h
    processcreator.cpp processcreator.h
    processexception.cpp processexception.h
    processhandle.h
    processstartedevent.cpp processstartedevent.h
    readmessageblock.cpp readmessageblock.h
    referencesmessage.cpp referencesmessage.h
    requestannotationsmessage.cpp requestannotationsmessage.h
    requestcompletionsmessage.cpp requestcompletionsmessage.h
    requestfollowsymbolmessage.cpp requestfollowsymbolmessage.h
    requestreferencesmessage.cpp requestreferencesmessage.h
    requesttooltipmessage.cpp requesttooltipmessage.h
    sourcelocationcontainer.cpp sourcelocationcontainer.h
    sourcelocationscontainer.cpp sourcelocationscontainer.h
    sourcerangecontainer.cpp sourcerangecontainer.h
    tokeninfocontainer.cpp tokeninfocontainer.h
    tooltipinfo.cpp tooltipinfo.h
    tooltipmessage.cpp tooltipmessage.h
    unsavedfilesremovedmessage.cpp unsavedfilesremovedmessage.h
    unsavedfilesupdatedmessage.cpp unsavedfilesupdatedmessage.h
    utf8string.cpp utf8string.h
    utf8stringvector.cpp utf8stringvector.h
    writemessageblock.cpp writemessageblock.h
)

if (NOT TARGET libclang)
  return()
endif()
