add_qtc_test(tst_qdoc
  DEPENDS Utils
  DEFINES "QDOC_TEST_CORPUS_DIR=\"${PROJECT_SOURCE_DIR}/doc\""
  INCLUDES "${PROJECT_SOURCE_DIR}/src/plugins/qdoc"
  SOURCES
    tst_qdoc.cpp
    "${PROJECT_SOURCE_DIR}/src/plugins/qdoc/qdocconfig.cpp"
    "${PROJECT_SOURCE_DIR}/src/plugins/qdoc/qdocparser.cpp"
    "${PROJECT_SOURCE_DIR}/src/plugins/qdoc/qdocrenderer.cpp"
)
