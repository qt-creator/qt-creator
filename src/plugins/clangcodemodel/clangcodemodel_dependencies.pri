QTC_PLUGIN_NAME = ClangCodeModel
QTC_LIB_DEPENDS += \
    utils \
    clangsupport
QTC_PLUGIN_DEPENDS += \
    coreplugin \
    cpptools \
    languageclient \
    texteditor
QTC_TEST_DEPENDS += \
    cppeditor \
    qmakeprojectmanager
