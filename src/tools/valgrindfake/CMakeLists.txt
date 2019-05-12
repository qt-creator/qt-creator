add_qtc_executable(valgrind-fake
  DEPENDS Qt5::Network Qt5::Xml
  SOURCES main.cpp outputgenerator.cpp outputgenerator.h
)

