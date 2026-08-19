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
    modellistwidget.cpp modellistwidget.h
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
    conversationmessage.h
)

# ---------------------------------------------------------------------------
# MCP server executable name — single source of truth for CMake and C++
# ---------------------------------------------------------------------------
set(_mcp_server_dir "qml_mcp_server")
set(_mcp_server_exe "qml_mcp_server")

if(WIN32)
  string(APPEND _mcp_server_exe ".exe")
endif()

target_compile_definitions(QmlDesignerMcp PRIVATE
  QML_MCP_SERVER_EXECUTABLE="${_mcp_server_dir}/${_mcp_server_exe}"
)

# ---------------------------------------------------------------------------
# Convert qml_mcp_server.py to executable and install it
# ---------------------------------------------------------------------------
env_with_default(QTC_BUILD_QML_MCP_SERVER ENV_QTC_BUILD_QML_MCP_SERVER OFF)
option(BUILD_QML_MCP_SERVER
       "Package qml_mcp_server.py into a standalone binary via PyInstaller"
       ${ENV_QTC_BUILD_QML_MCP_SERVER})

if(BUILD_QML_MCP_SERVER)
  set(_mcp_project_dir "${CMAKE_CURRENT_SOURCE_DIR}/mcp")
  set(_mcp_venv_dir "${CMAKE_CURRENT_BINARY_DIR}/mcp-venv")

  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${_mcp_project_dir}/pyproject.toml"
    "${_mcp_project_dir}/uv.lock"
  )

  find_program(UV_EXECUTABLE uv)
  if(UV_EXECUTABLE)
    if(WIN32)
      set(_mcp_python "${_mcp_venv_dir}/Scripts/python.exe")
    else()
      set(_mcp_python "${_mcp_venv_dir}/bin/python")
    endif()

    set(ENV{UV_PROJECT_ENVIRONMENT} "${_mcp_venv_dir}")
    execute_process(
      COMMAND "${UV_EXECUTABLE}" sync --locked --managed-python --group build
              --project "${_mcp_project_dir}"
      RESULT_VARIABLE _uv_sync_check
    )
    unset(ENV{UV_PROJECT_ENVIRONMENT})
    if(NOT _uv_sync_check EQUAL 0)
      message(FATAL_ERROR
        "Could not provision the QML MCP server runtime from '${_mcp_project_dir}'."
        " A stale lock file, an unusable '${UV_EXECUTABLE}', or no route to the"
        " Python and package indexes all fail here."
      )
    endif()
  else()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(_mcp_python "${Python3_EXECUTABLE}")

    execute_process(
      COMMAND "${_mcp_python}" -m PyInstaller --version
      RESULT_VARIABLE _pyinstaller_check
      OUTPUT_QUIET
      ERROR_QUIET
    )
    if(NOT _pyinstaller_check EQUAL 0)
      message(FATAL_ERROR
        "PyInstaller was not found in the Python environment '${_mcp_python}'."
        " Install it with:\n  ${_mcp_python} -m pip install pyinstaller"
      )
    endif()

    message(WARNING
      "'uv' was not found, so the server is built from the ambient Python"
      " '${_mcp_python}'. Its runtime version and payload differ from the"
      " shipped bundle, which is pinned in '${_mcp_project_dir}'. This build"
      " must not be packaged."
    )
  endif()

  message(STATUS "Using '${_mcp_python}' to generate the QML MCP server.")

  execute_process(
    COMMAND "${_mcp_python}" -c "import platform; print(platform.python_version())"
    RESULT_VARIABLE _python_version_check
    OUTPUT_VARIABLE _python_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(NOT _python_version_check EQUAL 0)
    message(FATAL_ERROR
      "Could not determine the Python version of '${_mcp_python}'."
    )
  endif()

  execute_process(
    COMMAND "${_mcp_python}" -c
            "from importlib.metadata import version; print(version('mcp'))"
    RESULT_VARIABLE _mcp_package_check
    OUTPUT_VARIABLE _mcp_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(NOT _mcp_package_check EQUAL 0)
    message(FATAL_ERROR
      "The 'mcp' package is missing from the environment of '${_mcp_python}'."
    )
  endif()

  execute_process(
    COMMAND "${_mcp_python}" -c "import ssl; print(ssl.OPENSSL_VERSION.split()[1])"
    RESULT_VARIABLE _openssl_version_check
    OUTPUT_VARIABLE _openssl_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(NOT _openssl_version_check EQUAL 0)
    message(FATAL_ERROR
      "Could not determine the OpenSSL version used by '${_mcp_python}'."
    )
  endif()

  add_custom_target(QmlDesignerMcpServer ALL
    COMMAND "${_mcp_python}" -m PyInstaller
            --distpath "${QtCreator_BINARY_DIR}/${IDE_LIBEXEC_PATH}"
            --workpath "${CMAKE_CURRENT_BINARY_DIR}/pyinstaller-build"
            --noconfirm
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/qml_mcp_server.spec"
    BYPRODUCTS
      "${QtCreator_BINARY_DIR}/${IDE_LIBEXEC_PATH}/${_mcp_server_dir}/${_mcp_server_exe}"
    VERBATIM
  )

  install(DIRECTORY
          "${QtCreator_BINARY_DIR}/${IDE_LIBEXEC_PATH}/${_mcp_server_dir}/"
          DESTINATION "${IDE_LIBEXEC_PATH}/${_mcp_server_dir}"
          COMPONENT Dependencies
          USE_SOURCE_PERMISSIONS)

  qtc_add_sbom(QmlDesignerMcpServerPython
    SBOM_ENTITY_TYPE THIRD_PARTY_LIBRARY
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_PATHS "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS qml-mcp-server-python
    PACKAGE_VERSION "${_python_version}"
  )

  qtc_add_sbom(QmlDesignerMcpServerMcp
    SBOM_ENTITY_TYPE THIRD_PARTY_LIBRARY
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_PATHS "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS qml-mcp-server-mcp
    PACKAGE_VERSION "${_mcp_version}"
  )

  qtc_add_sbom(QmlDesignerMcpServerOpenSSL
    SBOM_ENTITY_TYPE THIRD_PARTY_LIBRARY
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_PATHS "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS qml-mcp-server-openssl
    PACKAGE_VERSION "${_openssl_version}"
  )
endif()
