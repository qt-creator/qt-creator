TARGET = UpdateInfo
TEMPLATE = lib
QT += network xml

HEADERS += updateinfoplugin.h \
    updateinfo_global.h \
    updateinfobutton.h
SOURCES += updateinfoplugin.cpp \
    updateinfobutton.cpp
FORMS +=
RESOURCES += updateinfo.qrc

PROVIDER = Nokia

isEmpty(UPDATEINFO_DISABLE):UPDATEINFO_DISABLE=$$(UPDATEINFO_DISABLE)
isEmpty(UPDATEINFO_DISABLE):UPDATEINFO_DISABLE = "true"
else:UPDATEINFO_DISABLE = "false"

include(../../qtcreatorplugin.pri)

include(updateinfo_dependencies.pri)
