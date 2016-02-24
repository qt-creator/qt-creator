QTC_PLUGIN_NAME = ClangStaticAnalyzer
QTC_LIB_DEPENDS += \
    extensionsystem \
    utils
QTC_PLUGIN_DEPENDS += \
    debugger \
    cpptools
QTC_TEST_DEPENDS += \
    qbsprojectmanager \
    qmakeprojectmanager
