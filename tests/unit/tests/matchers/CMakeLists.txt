add_qtc_library(TestMatchers OBJECT
  EXCLUDE_FROM_INSTALL
  PROPERTIES SKIP_AUTOGEN ON
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  DEPENDS
    Googletest Utils TestDesignerCore
  SOURCES
    info_exportedtypenames-matcher.h
    import-matcher.h
    strippedstring-matcher.h
    unittest-matchers.h
    version-matcher.h
    qvariant-matcher.h
)
