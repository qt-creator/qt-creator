add_qtc_test(tst_devicectlutils
  DEPENDS Utils
  INCLUDES "${PROJECT_SOURCE_DIR}/src/plugins"
  SOURCES
    ../../../../src/plugins/ios/devicectlutils.cpp
    ../../../../src/plugins/ios/devicectlutils.h
    ../../../../src/plugins/ios/deviceinfo.cpp
    ../../../../src/plugins/ios/deviceinfo.h
    tst_devicectlutils.cpp
)
