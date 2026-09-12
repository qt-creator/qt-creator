add_qtc_executable(buildoutputparser
  DEPENDS Utils ProjectExplorer QtSupport QmakeProjectManager
  SOURCES
    main.cpp
    outputprocessor.cpp outputprocessor.h
)

qtc_add_version_resource(buildoutputparser "${IDE_DISPLAY_NAME} Build Output Parser")
