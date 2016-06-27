QTC_PLUGIN_NAME = AutoTest

QTC_PLUGIN_DEPENDS += \
    coreplugin \
    projectexplorer \
    cpptools \
    qmljstools \
    debugger

QTC_LIB_DEPENDS += \
    cplusplus \
    qmljs \
    utils

QTC_TEST_DEPENDS += \
    qbsprojectmanager \
    qmakeprojectmanager \
    qtsupport

#QTC_PLUGIN_RECOMMENDS += \
