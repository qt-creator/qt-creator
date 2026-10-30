add_qtc_test(tst_voiceinput
  INCLUDES "${PROJECT_SOURCE_DIR}/src/libs/voiceinput"
  SOURCES
    tst_voiceinput.cpp
    "${PROJECT_SOURCE_DIR}/src/libs/voiceinput/speechactivity.cpp"
)
