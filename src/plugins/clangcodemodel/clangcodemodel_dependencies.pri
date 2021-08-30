QTC_PLUGIN_NAME = ClangCodeModel
QTC_LIB_DEPENDS += \
    utils \
    clangsupport
QTC_PLUGIN_DEPENDS += \
    coreplugin \
    cppeditor \
    languageclient \
    texteditor
QTC_TEST_DEPENDS += \
    qmakeprojectmanager

equals(TEST, 1): QTC_PLUGIN_DEPENDS += qtsupport
