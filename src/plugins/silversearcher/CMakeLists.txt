add_qtc_plugin(SilverSearcher
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    findinfilessilversearcher.cpp findinfilessilversearcher.h
    silversearcheroutputparser.cpp silversearcheroutputparser.h
    silversearcherplugin.cpp silversearcherplugin.h
)

extend_qtc_plugin(SilverSearcher CONDITION WITH_TESTS
  SOURCES
    outputparser_test.cpp outputparser_test.h
)
