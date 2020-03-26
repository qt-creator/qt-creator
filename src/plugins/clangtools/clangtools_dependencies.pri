QTC_PLUGIN_NAME = ClangTools
QTC_LIB_DEPENDS += \
    extensionsystem \
    utils

include(../../shared/yaml-cpp/yaml-cpp_installation.pri)
isEmpty(EXTERNAL_YAML_CPP_FOUND): QTC_LIB_DEPENDS += yaml-cpp

QTC_PLUGIN_DEPENDS += \
    debugger \
    cpptools
QTC_PLUGIN_RECOMMENDS += \
    cppeditor
QTC_TEST_DEPENDS += \
    qbsprojectmanager \
    qmakeprojectmanager
