add_qtc_test(tst_minihttpserver
  DEPENDS Qt::Network
  INCLUDES "${PROJECT_SOURCE_DIR}/src/libs"
  SOURCES
    tst_minihttpserver.cpp
    "${PROJECT_SOURCE_DIR}/src/libs/mcp/server/minihttpserver.h"
)

add_qtc_test(tst_toolvalidation
  DEPENDS McpServerLib Utils
  SOURCES tst_toolvalidation.cpp
)
