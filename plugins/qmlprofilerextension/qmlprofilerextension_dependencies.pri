QTC_PLUGIN_NAME = QmlProfilerExtension
#QTC_LIB_DEPENDS += \
#    qmldebug \
#    extensionsystem
QTC_PLUGIN_DEPENDS += \
    qmlprofiler

CONFIG(licensechecker): QT_PLUGIN_DEPENDS += licensechecker
