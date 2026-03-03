find_package(Qt6 6.11 COMPONENTS HttpServer OPTIONAL QUIET)

add_qtc_library(McpServerLib
  PUBLIC_DEPENDS Qt::Core Qt::Network Utils
  SOURCES
    mcpserver.cpp
    mcpserver.h
    mcpserver_global.h
    minihttpserver.h
    ../schemas/schema_2025_11_25.cpp
    ../schemas/schema_2025_11_25.h
)

extend_qtc_library(McpServerLib
  CONDITION TARGET Qt::HttpServer
  PUBLIC_DEPENDS Qt::HttpServer
  DEFINES MCP_SERVER_HAS_QT_HTTP_SERVER
)

if (MSVC)
    # Prevent fatal error C1128 on large generated schema
    target_compile_options(McpServerLib PRIVATE /bigobj)
endif()
if (MINGW)
    target_compile_options(McpServerLib PRIVATE "-Wa,-mbig-obj")
endif()
