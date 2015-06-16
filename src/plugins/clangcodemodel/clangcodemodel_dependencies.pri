QTC_PLUGIN_NAME = ClangCodeModel
QTC_LIB_DEPENDS += \
    utils \
    clangbackendipc
QTC_PLUGIN_DEPENDS += \
    coreplugin \
    cpptools \
    texteditor
QTC_TEST_DEPENDS += \
    cppeditor \
    qmakeprojectmanager
