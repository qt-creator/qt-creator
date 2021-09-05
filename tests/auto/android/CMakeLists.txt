add_qtc_test(tst_avdmanageroutputparser
  DEPENDS Utils
  INCLUDES
    "${PROJECT_SOURCE_DIR}/src/plugins"
    "${PROJECT_SOURCE_DIR}/src/plugins/android"
  SOURCES
    tst_avdmanageroutputparser.cpp
    "${PROJECT_SOURCE_DIR}/src/plugins/android/avdmanageroutputparser.cpp"
    "${PROJECT_SOURCE_DIR}/src/plugins/android/avdmanageroutputparser.h"
    "${PROJECT_SOURCE_DIR}/src/plugins/android/androiddeviceinfo.cpp"
    "${PROJECT_SOURCE_DIR}/src/plugins/android/androiddeviceinfo.h"
    "${PROJECT_SOURCE_DIR}/src/plugins/android/androidmanager.cpp"
    "${PROJECT_SOURCE_DIR}/src/plugins/android/androidmanager.h"
)

qtc_add_resources(tst_avdmanageroutputparser tst_avdmanageroutputparser_rcc
  FILES
    Test.avd/config.ini
    TestTablet.avd/config.ini
)
