add_qtc_library(TestQtPrinters OBJECT
  EXCLUDE_FROM_INSTALL
  PROPERTIES SKIP_AUTOGEN ON
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  PUBLIC_DEPENDS
    Qt::Core Qt::Widgets Googletest
  SOURCES
    gtest-qt-printing.cpp gtest-qt-printing.h
    gtest-std-printing.h
)

add_qtc_library(TestPrinters OBJECT
  EXCLUDE_FROM_INSTALL
  PROPERTIES SKIP_AUTOGEN ON
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  PUBLIC_DEPENDS
    Qt::Core Qt::Widgets Sqlite Googletest TestDesignerCore TestQtPrinters
  SOURCES
    gtest-creator-printing.cpp gtest-creator-printing.h
    gtest-qt-printing.cpp gtest-qt-printing.h
    gtest-std-printing.h
)
