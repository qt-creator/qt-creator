QTC_PLUGIN_NAME = QbsProjectManager
# The Qbs libraries require special code and can not be covered here!
QTC_LIB_DEPENDS += \
    extensionsystem \
    qmljs
QTC_PLUGIN_DEPENDS += \
    coreplugin \
    projectexplorer \
    cpptools \
    qtsupport \
    qmljstools \
    resourceeditor
