add_qtc_plugin(SilverSearcher
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    findinfilessilversearcher.cpp findinfilessilversearcher.h
    silversearcheroutputparser.cpp silversearcheroutputparser.h
    silversearcherplugin.cpp silversearcherplugin.h
)

if (WITH_TESTS)
  target_sources(SilverSearcher PRIVATE
    outputparser_test.cpp outputparser_test.h
  )
endif()
