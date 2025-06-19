add_qtc_plugin(Learning
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    learningplugin.cpp
    learningsettings.h learningsettings.cpp
    onboardingwizard.h onboardingwizard.cpp
    overviewwelcomepage.h overviewwelcomepage.cpp
    qtacademywelcomepage.h qtacademywelcomepage.cpp
)

extend_qtc_plugin(Learning
  CONDITION WITH_TESTS
  SOURCES
     learning_test.qrc
)

file(GLOB images RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} overview/*.webp overview/*.json)
qtc_add_resources(Learning "overview"
  PREFIX "/learning"
  BASE "."
  FILES
    ${images}
)
