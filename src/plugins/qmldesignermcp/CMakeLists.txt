add_qtc_plugin(QmlDesignerMcp
  CONDITION TARGET QtCreator::QmlDesigner
  PLUGIN_DEPENDS
    QtCreator::QmlDesigner QtCreator::QmlDesignerBase
  DEPENDS
    Qt6::Qml
    AstCheck
    DEFINES SHARE_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../../share/qtcreator/qmldesigner"
  SOURCES
    aiassistantconstants.h
    aiassistantview.cpp aiassistantview.h
    aiassistantwidget.cpp aiassistantwidget.h
    aiassistanttermsdialog.cpp aiassistanttermsdialog.h
    aiassistantutils.cpp aiassistantutils.h
    aiassistant.qrc
    aimodelsmodel.cpp aimodelsmodel.h
    airesponse.cpp airesponse.h
    aitransaction.cpp aitransaction.h
    chathistorymodel.cpp chathistorymodel.h
    chatmessage.cpp chatmessage.h
    qmldesignermcpplugin.cpp qmldesignermcpplugin.h
)

extend_qtc_plugin(QmlDesignerMcp
  PUBLIC_INCLUDES settings
  SOURCES_PREFIX settings
  SOURCES
    aiproviderconfig.cpp aiproviderconfig.h
    aidefaultsettings.cpp aidefaultsettings.h
    aiprovidersettings.cpp aiprovidersettings.h
    aiprovidersettingswidget.cpp aiprovidersettingswidget.h
    mcpserverconfigstore.cpp mcpserverconfigstore.h
    mcpsettings.cpp mcpsettings.h
    mcpserversettingswidget.cpp mcpserversettingswidget.h
    stringlistwidget.cpp stringlistwidget.h
)

extend_qtc_plugin(QmlDesignerMcp
  PUBLIC_INCLUDES api
  SOURCES_PREFIX api
  SOURCES
    aiapiadapter.h
    claudeapiadapter.cpp claudeapiadapter.h
    geminiapiadapter.cpp geminiapiadapter.h
    openaiapiresponseadapter.cpp openaiapiresponseadapter.h
)

extend_qtc_plugin(QmlDesignerMcp
  PUBLIC_INCLUDES mcp
  SOURCES_PREFIX mcp
  SOURCES
    mcpclient.cpp mcpclient.h
    mcpclientstdio.cpp mcpclientstdio.h
    mcpclienthttp.cpp mcpclienthttp.h
    mcphost.cpp mcphost.h
    agenticrequestmanager.cpp agenticrequestmanager.h
    conversationmanager.cpp conversationmanager.h
    toolregistry.cpp toolregistry.h
    conversationturn.h
)

# ---------------------------------------------------------------------------
# MCP server executable name — single source of truth for CMake and C++
# ---------------------------------------------------------------------------
if(WIN32)
  set(_mcp_server_name "qml_mcp_server.exe")
else()
  set(_mcp_server_name "qml_mcp_server")
endif()

target_compile_definitions(QmlDesignerMcp PRIVATE
  QML_MCP_SERVER_EXECUTABLE="${_mcp_server_name}"
)

# ---------------------------------------------------------------------------
# Convert qml_mcp_server.py to executable and install it
# ---------------------------------------------------------------------------
env_with_default(QTC_BUILD_QML_MCP_SERVER ENV_QTC_BUILD_QML_MCP_SERVER OFF)
option(BUILD_QML_MCP_SERVER
       "Package qml_mcp_server.py into a standalone binary via PyInstaller"
       ${ENV_QTC_BUILD_QML_MCP_SERVER})

if(BUILD_QML_MCP_SERVER)
  set(_mcp_server_src "${CMAKE_CURRENT_SOURCE_DIR}/mcp/qml_mcp_server.py")

  if(NOT EXISTS "${_mcp_server_src}")
    message(FATAL_ERROR
      "'${_mcp_server_src}' was not found."
    )
  endif()

  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  message(STATUS "Using Python3_EXECUTABLE='${Python3_EXECUTABLE}' to generate the QML MCP server.")

  execute_process(
    COMMAND "${Python3_EXECUTABLE}" -m PyInstaller --version
    RESULT_VARIABLE _pyinstaller_check
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(NOT _pyinstaller_check EQUAL 0)
    message(FATAL_ERROR
      "PyInstaller was not found in the Python environment '${Python3_EXECUTABLE}'."
      " Install it with:\n  ${Python3_EXECUTABLE} -m pip install pyinstaller"
    )
  endif()

  execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "import mcp"
    RESULT_VARIABLE _mcp_package_check
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(NOT _mcp_package_check EQUAL 0)
    message(FATAL_ERROR
      "The 'mcp' Python package was not found in the Python environment '${Python3_EXECUTABLE}'."
      " Install it with:\n  ${Python3_EXECUTABLE} -m pip install mcp"
    )
  endif()

  add_custom_target(QmlDesignerMcpServer ALL
    COMMAND "${Python3_EXECUTABLE}" -m PyInstaller
            --onefile
            --distpath "${QtCreator_BINARY_DIR}/${IDE_LIBEXEC_PATH}"
            "${_mcp_server_src}"
    BYPRODUCTS "${QtCreator_BINARY_DIR}/${IDE_LIBEXEC_PATH}/${_mcp_server_name}"
    VERBATIM
  )

  install(PROGRAMS "${QtCreator_BINARY_DIR}/${IDE_LIBEXEC_PATH}/${_mcp_server_name}"
          DESTINATION "${IDE_LIBEXEC_PATH}"
          COMPONENT Dependencies)
endif()
