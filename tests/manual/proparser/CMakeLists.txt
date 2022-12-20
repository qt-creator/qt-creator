add_qtc_test(tst_manual_proparser
  MANUALTEST
  DEPENDS Qt::Core Qt::Core5Compat
  DEFINES
    "QMAKE_BUILTIN_PRFS"
    "QT_NO_CAST_TO_ASCII"
    "QT_RESTRICTED_CAST_FROM_ASCII"
    "QT_USE_QSTRINGBUILDER"
    "PROEVALUATOR_FULL"
    "PROEVALUATOR_CUMULATIVE"
    "PROEVALUATOR_INIT_PROPS"
  INCLUDES
    "${PROJECT_SOURCE_DIR}/src/shared/proparser/"
    "${PROJECT_SOURCE_DIR}/src/libs/"
  SOURCES
    main.cpp
)

extend_qtc_test(tst_manual_proparser
  SOURCES_PREFIX "${PROJECT_SOURCE_DIR}/src/shared/proparser/"
  SOURCES
    ioutils.cpp ioutils.h
    profileevaluator.cpp profileevaluator.h
    proitems.cpp proitems.h
    proparser.qrc
    qmake_global.h
    qmakebuiltins.cpp
    qmakeevaluator.cpp qmakeevaluator.h qmakeevaluator_p.h
    qmakeglobals.cpp qmakeglobals.h
    qmakeparser.cpp qmakeparser.h
    qmakevfs.cpp qmakevfs.h
    registry.cpp registry_p.h
)

extend_qtc_test(tst_manual_proparser CONDITION WIN32
    DEPENDS advapi32
)

extend_qtc_test(tst_manual_proparser
  PROPERTIES
    OUTPUT_NAME "testreader"
)
