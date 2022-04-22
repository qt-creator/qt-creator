add_qtc_test(tst_updateinfo
  INCLUDES ${PROJECT_SOURCE_DIR}/src/plugins
  DEPENDS Utils Qt5::Xml
  SOURCES tst_updateinfo.cpp
)
