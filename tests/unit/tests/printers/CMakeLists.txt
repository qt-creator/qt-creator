add_qtc_library(TestPrinters OBJECT
  EXCLUDE_FROM_INSTALL
  SKIP_AUTOMOC ON
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  DEPENDS
    Qt::Core Qt::Widgets Sqlite Googletest TestDesignerCore
  SOURCES
    gtest-creator-printing.cpp gtest-creator-printing.h
    gtest-qt-printing.cpp gtest-qt-printing.h
    gtest-std-printing.h
)
