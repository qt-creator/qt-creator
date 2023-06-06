add_qtc_plugin(SilverSearcher
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    findinfilessilversearcher.cpp findinfilessilversearcher.h
    silversearcherparser.cpp silversearcherparser.h
    silversearcherplugin.cpp silversearcherplugin.h
    silversearchertr.h
)

extend_qtc_plugin(SilverSearcher CONDITION WITH_TESTS
  SOURCES
    silversearcherparser_test.cpp silversearcherparser_test.h
)
