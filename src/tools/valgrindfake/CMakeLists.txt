add_qtc_executable(valgrind-fake
  SKIP_INSTALL
  DEPENDS Qt::Network Qt::Xml
  SOURCES main.cpp outputgenerator.cpp outputgenerator.h
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

