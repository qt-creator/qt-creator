QTC_PLUGIN_NAME = CppTools
QTC_LIB_DEPENDS += \
    cplusplus \
    clangsupport \
    extensionsystem \
    utils
QTC_PLUGIN_DEPENDS += \
    coreplugin \
    projectexplorer \
    texteditor
QTC_TEST_DEPENDS += \
    cppeditor \
    qmakeprojectmanager
