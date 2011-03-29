TEMPLATE = app
TARGET = simple_gdbtest_app
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = .

SOURCES +=  simple_gdbtest_app.cpp

QT += network
QT += script
QT += xml
#unix: QMAKE_CXXFLAGS += -msse2
#DEFINES += USE_BOOST=1

message("this says <foo & bar>")

maemo5 {
    target.path = /opt/usr/lib
    INSTALLS += target
}
