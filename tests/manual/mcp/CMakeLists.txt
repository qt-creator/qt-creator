find_package(Qt6 6.5 COMPONENTS HttpServer OPTIONAL QUIET)

add_qtc_test(tst_mcpserver
  MANUALTEST
  CONDITION TARGET McpServerLib
  DEPENDS
    McpServerLib Qt::HttpServer
  SOURCES
    tst_mcpserver.cpp
)
