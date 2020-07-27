SOURCES += \
    ../common/themeselector.cpp \
    tst_manual_widgets_infolabel.cpp

HEADERS += \
    ../common/themeselector.h

RESOURCES += \
    ../common/themes.qrc

QTC_LIB_DEPENDS += \
    utils

QTC_PLUGIN_DEPENDS += \
    coreplugin

include(../../../auto/qttest.pri)
