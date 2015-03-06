QTC_PLUGIN_NAME = QtSupport
QTC_LIB_DEPENDS += \
    extensionsystem \
    qmljs \
    utils
QTC_PLUGIN_DEPENDS += \
    coreplugin \
    cpptools \
    projectexplorer
DEFINES *= \
    QMAKE_AS_LIBRARY \
    PROPARSER_THREAD_SAFE \
    PROEVALUATOR_THREAD_SAFE \
    PROEVALUATOR_CUMULATIVE \
    PROEVALUATOR_SETENV
INCLUDEPATH *= $$PWD/../../shared
