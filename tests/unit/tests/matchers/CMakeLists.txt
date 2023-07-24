add_qtc_library(TestMatchers OBJECT
  EXCLUDE_FROM_INSTALL
  PROPERTIES SKIP_AUTOGEN ON
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  CONDITION QmlDesigner
  DEPENDS
    Googletest Utils QmlDesigner
  SOURCES
    info_exportedtypenames-matcher.h
    import-matcher.h
    unittest-matchers.h
    version-matcher.h
    qvariant-matcher.h
)
