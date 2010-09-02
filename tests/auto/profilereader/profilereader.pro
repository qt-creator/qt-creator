TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
CONFIG += qtestlib testcase

SOURCES += \
    tst_profilereader.cpp

HEADERS += \
    profilecache.h \
    qtversionmanager.h

TARGET=tst_$$TARGET
