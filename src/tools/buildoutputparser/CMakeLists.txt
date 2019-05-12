add_qtc_executable(buildoutputparser
  DEPENDS Utils ProjectExplorer QtSupport QmakeProjectManager
  SOURCES
    main.cpp
    outputprocessor.cpp outputprocessor.h
)
