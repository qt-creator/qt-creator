TEMPLATE = app
TARGET = simple_test_app
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = .

SOURCES +=  simple_test_app.cpp

QT += network
QT += script
QT += xml
QT += core-private
#unix: QMAKE_CXXFLAGS += -msse2
#DEFINES += USE_BOOST=1

message("this says <foo & bar>")

maemo5 {
    target.path = /opt/usr/lib
    INSTALLS += target
}
