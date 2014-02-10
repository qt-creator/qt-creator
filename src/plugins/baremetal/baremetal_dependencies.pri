QTC_PLUGIN_NAME = BareMetal
QTC_LIB_DEPENDS += \
    extensionsystem \
    ssh \
    utils
QTC_PLUGIN_DEPENDS += \
    coreplugin \
    debugger \
    projectexplorer \
    qtsupport

QTC_PLUGIN_RECOMMENDS += \
    # optional plugin dependencies. nothing here at this time

