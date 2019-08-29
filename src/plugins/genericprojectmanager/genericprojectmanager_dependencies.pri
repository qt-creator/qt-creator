QTC_PLUGIN_NAME = GenericProjectManager
QTC_LIB_DEPENDS += \
    extensionsystem \
    utils
QTC_PLUGIN_DEPENDS += \
    coreplugin \
    projectexplorer \
    texteditor \
    qtsupport
QTC_PLUGIN_RECOMMENDS += \
    cpptools
QTC_TEST_DEPENDS += \
    cppeditor
