add_qtc_library(ProParser SHARED
  DEPENDS Qt5::Core Utils
  PUBLIC_DEFINES
    QMAKE_BUILTIN_PRFS QMAKE_OVERRIDE_PRFS
    PROPARSER_THREAD_SAFE PROEVALUATOR_CUMULATIVE PROEVALUATOR_DUAL_VFS
    PROEVALUATOR_SETENV PROEVALUATOR_THREAD_SAFE
  SOURCES
    ioutils.cpp ioutils.h
    profileevaluator.cpp profileevaluator.h
    proitems.cpp proitems.h
    proparser.qrc
    prowriter.cpp prowriter.h
    qmake_global.h
    qmakebuiltins.cpp
    qmakeevaluator.cpp qmakeevaluator.h qmakeevaluator_p.h
    qmakeglobals.cpp qmakeglobals.h
    qmakeparser.cpp qmakeparser.h
    qmakevfs.cpp qmakevfs.h
    registry.cpp registry_p.h
  PROPERTIES
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

extend_qtc_library(ProParser
  CONDITION WIN32
  DEFINES _UNICODE UNICODE
)
