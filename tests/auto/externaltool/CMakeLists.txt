add_qtc_test(tst_externaltooltest
  DEPENDS Core app_version
  SOURCES
    tst_externaltooltest.cpp
    "${PROJECT_SOURCE_DIR}/src/plugins/coreplugin/externaltool.cpp"
    "${PROJECT_SOURCE_DIR}/src/plugins/coreplugin/externaltool.h"
)
