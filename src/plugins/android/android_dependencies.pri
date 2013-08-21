QTC_PLUGIN_NAME = Android
QTC_PLUGIN_DEPENDS += \
    coreplugin \
    debugger \
    projectexplorer \
    qt4projectmanager \
    qtsupport \
    texteditor \
    analyzerbase

exists(../../shared/qbs/qbs.pro): \
    QTC_PLUGIN_DEPENDS += \
        qbsprojectmanager
