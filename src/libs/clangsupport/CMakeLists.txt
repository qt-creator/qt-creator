set(CLANG_VERSION ${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH})

add_qtc_library(ClangSupport
  DEPENDS Threads::Threads
  PUBLIC_DEPENDS Utils Sqlite Qt5::Core Qt5::Network
  PUBLIC_DEFINES
    CLANG_VERSION="${CLANG_VERSION}"
    CLANG_RESOURCE_DIR="${IDE_LIBEXEC_PATH}/clang/lib/clang/${CLANG_VERSION}/include"
    CLANG_BINDIR="${IDE_LIBEXEC_PATH}/clang/bin"
  DEFINES CLANGSUPPORT_BUILD_LIB
  PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}"
  SOURCES
    alivemessage.cpp alivemessage.h
    annotationsmessage.cpp annotationsmessage.h
    baseserverproxy.cpp baseserverproxy.h
    cancelmessage.cpp cancelmessage.h
    clangcodemodelclientinterface.cpp clangcodemodelclientinterface.h
    clangcodemodelclientmessages.h
    clangcodemodelclientproxy.cpp clangcodemodelclientproxy.h
    clangcodemodelconnectionclient.cpp clangcodemodelconnectionclient.h
    clangcodemodelserverinterface.cpp clangcodemodelserverinterface.h
    clangcodemodelservermessages.h
    clangcodemodelserverproxy.cpp clangcodemodelserverproxy.h
    clangpathwatcher.h
    clangpathwatcherinterface.h
    clangpathwatchernotifier.h
    clangrefactoringclientmessages.h
    clangrefactoringmessages.h
    clangrefactoringservermessages.h
    clangsupport_global.h
    clangsupportdebugutils.cpp clangsupportdebugutils.h
    clangsupportexceptions.h
    codecompletion.cpp codecompletion.h
    codecompletionchunk.cpp codecompletionchunk.h
    commandlinebuilder.h
    compilermacro.h
    completionsmessage.cpp completionsmessage.h
    connectionclient.cpp connectionclient.h
    connectionserver.cpp connectionserver.h
    diagnosticcontainer.cpp diagnosticcontainer.h
    directoryandfilepathid.h
    directorypathid.h
    documentschangedmessage.cpp documentschangedmessage.h
    documentsclosedmessage.cpp documentsclosedmessage.h
    documentsopenedmessage.cpp documentsopenedmessage.h
    documentvisibilitychangedmessage.cpp documentvisibilitychangedmessage.h
    dynamicastmatcherdiagnosticcontainer.cpp dynamicastmatcherdiagnosticcontainer.h
    dynamicastmatcherdiagnosticcontextcontainer.cpp dynamicastmatcherdiagnosticcontextcontainer.h
    dynamicastmatcherdiagnosticmessagecontainer.cpp dynamicastmatcherdiagnosticmessagecontainer.h
    dynamicmatcherdiagnostics.h
    echomessage.cpp echomessage.h
    endmessage.cpp endmessage.h
    environment.h
    executeinloop.h
    filecontainer.cpp filecontainer.h
    filecontainerv2.cpp filecontainerv2.h
    filepath.cpp filepath.h
    filepathcache.h
    filepathcaching.cpp filepathcaching.h
    filepathcachingfwd.h
    filepathcachinginterface.h
    filepathexceptions.h
    filepathid.cpp filepathid.h
    filepathstorage.h
    filepathstoragesources.h
    filepathstoragesqlitestatementfactory.h
    filepathview.h
    filestatus.h
    filestatuscache.cpp filestatuscache.h
    filesystem.cpp filesystem.h
    filesysteminterface.h
    fixitcontainer.cpp fixitcontainer.h
    followsymbolmessage.cpp followsymbolmessage.h
    generatedfiles.cpp generatedfiles.h
    generatedfilesinterface.h
    idpaths.h
    includesearchpath.h
    ipcclientinterface.h
    ipcclientprovider.h
    ipcinterface.h
    ipcserverinterface.h
    lineprefixer.cpp lineprefixer.h
    messageenvelop.cpp messageenvelop.h
    modifiedtimechecker.h
    modifiedtimecheckerinterface.h
    nativefilepath.h
    pchmanagerclientinterface.cpp pchmanagerclientinterface.h
    pchmanagerclientproxy.cpp pchmanagerclientproxy.h
    pchmanagerserverinterface.cpp pchmanagerserverinterface.h
    pchmanagerserverproxy.cpp pchmanagerserverproxy.h
    pchpaths.h
    precompiledheadersupdatedmessage.cpp precompiledheadersupdatedmessage.h
    processcreator.cpp processcreator.h
    processexception.cpp processexception.h
    processhandle.h
    processstartedevent.cpp processstartedevent.h
    progresscounter.h
    progressmessage.h
    projectmanagementserverinterface.h
    projectpartartefact.cpp projectpartartefact.h
    projectpartcontainer.cpp projectpartcontainer.h
    projectpartid.h
    projectpartpch.cpp projectpartpch.h
    projectpartsstorage.h
    projectpartsstorageinterface.h
    readmessageblock.cpp readmessageblock.h
    refactoringclientinterface.cpp refactoringclientinterface.h
    refactoringclientproxy.cpp refactoringclientproxy.h
    refactoringdatabaseinitializer.h
    refactoringserverinterface.cpp refactoringserverinterface.h
    refactoringserverproxy.cpp refactoringserverproxy.h
    referencesmessage.cpp referencesmessage.h
    removegeneratedfilesmessage.cpp removegeneratedfilesmessage.h
    removeprojectpartsmessage.h
    requestannotationsmessage.cpp requestannotationsmessage.h
    requestcompletionsmessage.cpp requestcompletionsmessage.h
    requestfollowsymbolmessage.cpp requestfollowsymbolmessage.h
    requestreferencesmessage.cpp requestreferencesmessage.h
    requestsourcerangesanddiagnosticsforquerymessage.cpp requestsourcerangesanddiagnosticsforquerymessage.h
    requestsourcerangesforquerymessage.cpp requestsourcerangesforquerymessage.h
    requesttooltipmessage.cpp requesttooltipmessage.h
    sourceentry.h
    sourcelocationcontainer.cpp sourcelocationcontainer.h
    sourcelocationcontainerv2.cpp sourcelocationcontainerv2.h
    sourcelocationscontainer.cpp sourcelocationscontainer.h
    sourcerangecontainer.cpp sourcerangecontainer.h
    sourcerangecontainerv2.cpp sourcerangecontainerv2.h
    sourcerangesanddiagnosticsforquerymessage.cpp sourcerangesanddiagnosticsforquerymessage.h
    sourcerangescontainer.cpp sourcerangescontainer.h
    sourcerangesforquerymessage.cpp sourcerangesforquerymessage.h
    sourcerangewithtextcontainer.cpp sourcerangewithtextcontainer.h
    stringcache.h
    stringcachealgorithms.h
    stringcachefwd.h
    tokeninfocontainer.cpp tokeninfocontainer.h
    tooltipinfo.cpp tooltipinfo.h
    tooltipmessage.cpp tooltipmessage.h
    unsavedfilesremovedmessage.cpp unsavedfilesremovedmessage.h
    unsavedfilesupdatedmessage.cpp unsavedfilesupdatedmessage.h
    updategeneratedfilesmessage.cpp updategeneratedfilesmessage.h
    updateprojectpartsmessage.cpp updateprojectpartsmessage.h
    writemessageblock.cpp writemessageblock.h
)

if (NOT TARGET libclang)
  return()
endif()

# For the developer build directory
qtc_copy_to_builddir(copy_clang_to_builddir
  DIRECTORIES "${LLVM_LIBRARY_DIR}/clang/${CLANG_VERSION}/include"
  DESTINATION "${IDE_LIBEXEC_PATH}/clang/lib/clang/${CLANG_VERSION}/include"
)

foreach(executable clang clang-cl clangd clang-tidy clazy-standalone)
  if (EXISTS "${LLVM_TOOLS_BINARY_DIR}/${executable}${CMAKE_EXECUTABLE_SUFFIX}")
    qtc_copy_to_builddir(copy_clang_${executable}_to_builddir
      FILES "${LLVM_TOOLS_BINARY_DIR}/${executable}${CMAKE_EXECUTABLE_SUFFIX}"
      DESTINATION "${IDE_LIBEXEC_PATH}/clang/bin/"
    )
    # For the install directory
    install(PROGRAMS
      "${LLVM_TOOLS_BINARY_DIR}/${executable}${CMAKE_EXECUTABLE_SUFFIX}"
      DESTINATION "${IDE_LIBEXEC_PATH}/clang/bin"
      COMPONENT Dependencies
      EXCLUDE_FROM_ALL
    )
    if (IS_SYMLINK "${LLVM_TOOLS_BINARY_DIR}/${executable}${CMAKE_EXECUTABLE_SUFFIX}")
      file(READ_SYMLINK
        "${LLVM_TOOLS_BINARY_DIR}/${executable}${CMAKE_EXECUTABLE_SUFFIX}" real_executable)
      install(PROGRAMS
        "${LLVM_TOOLS_BINARY_DIR}/${real_executable}${CMAKE_EXECUTABLE_SUFFIX}"
        DESTINATION "${IDE_LIBEXEC_PATH}/clang/bin"
        COMPONENT Dependencies
        EXCLUDE_FROM_ALL
      )
    endif()
  endif()
endforeach()

install(DIRECTORY ${LLVM_LIBRARY_DIR}/clang/${CLANG_VERSION}/include
  DESTINATION "${IDE_LIBEXEC_PATH}/clang/lib/clang/${CLANG_VERSION}"
  COMPONENT Dependencies
  EXCLUDE_FROM_ALL
)
