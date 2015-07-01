QTC_PLUGIN_NAME = ClangStaticAnalyzer
QTC_LIB_DEPENDS += \
    extensionsystem \
    utils
QTC_PLUGIN_DEPENDS += \
    analyzerbase \
    cpptools
QTC_TEST_DEPENDS += \
    qbsprojectmanager \
    qmakeprojectmanager

CONFIG(licensechecker): QTC_PLUGIN_DEPENDS += licensechecker
