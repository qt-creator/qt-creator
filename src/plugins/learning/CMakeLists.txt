add_qtc_plugin(Learning
  PLUGIN_DEPENDS Core
  SOURCES
    learningplugin.cpp
    qtacademywelcomepage.h qtacademywelcomepage.cpp
)

extend_qtc_plugin(Learning
  CONDITION WITH_TESTS
  SOURCES
     learning_test.qrc
)
