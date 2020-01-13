TARGET = Marketplace
TEMPLATE = lib
QT += network

include(../../qtcreatorplugin.pri)

DEFINES += MARKETPLACE_LIBRARY

SOURCES += \
    productlistmodel.cpp \
    qtmarketplacewelcomepage.cpp

HEADERS += \
    marketplaceplugin.h \
    productlistmodel.h \
    qtmarketplacewelcomepage.h
