include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../libs/qmljs/qmljs.pri)
include(../../libs/utils/utils.pri)
DEFINES *= \
    QMAKE_AS_LIBRARY \
    PROPARSER_THREAD_SAFE \
    PROEVALUATOR_THREAD_SAFE \
    PROEVALUATOR_CUMULATIVE \
    PROEVALUATOR_SETENV
