add_qtc_test(tst_externaltooltest
  DEPENDS Core app_version
  SOURCES
    tst_externaltooltest.cpp
    "${CMAKE_SOURCE_DIR}/src/plugins/coreplugin/externaltool.cpp"
    "${CMAKE_SOURCE_DIR}/src/plugins/coreplugin/externaltool.h"
)
