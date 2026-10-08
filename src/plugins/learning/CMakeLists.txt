add_qtc_plugin(Learning
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport ExtensionManager
  SOURCES
    learningplugin.cpp
    learningsettings.h learningsettings.cpp
    learningtr.h
    onboardingwizard.h onboardingwizard.cpp
    overviewwelcomepage.h overviewwelcomepage.cpp
    qtacademywelcomepage.h qtacademywelcomepage.cpp
)

qtc_add_resources(Learning "testcases"
  CONDITION WITH_TESTS
  PREFIX "/learning"
  FILES testdata/courses.json
)

file(GLOB images RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} overview/*.webp overview/*.json)
qtc_add_resources(Learning "overview"
  PREFIX "/learning"
  FILES
    ${images}
)
