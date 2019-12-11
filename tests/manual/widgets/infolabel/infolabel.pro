SOURCES += \
    tst_manual_widgets_infolabel.cpp

RESOURCES += \
    ../themes.qrc

QTC_LIB_DEPENDS += \
    utils

QTC_PLUGIN_DEPENDS += \
    coreplugin

include(../../../auto/qttest.pri)
