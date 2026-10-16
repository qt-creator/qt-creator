add_qtc_plugin(Ripgrep
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    findinfilesripgrep.cpp findinfilesripgrep.h
    ripgrepparser.cpp ripgrepparser.h
    ripgrepplugin.cpp
    ripgreptr.h
)

extend_qtc_plugin(Ripgrep CONDITION WITH_TESTS
  SOURCES
    ripgrepengine_test.cpp ripgrepengine_test.h
    ripgrepparser_test.cpp ripgrepparser_test.h
)
