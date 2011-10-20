TEMPLATE = app
TARGET = simple_test_app
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = .

SOURCES +=  simple_test_app.cpp

QT += network
QT += script
QT += xml
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += core-private
    QT *= widgets
}
#unix: QMAKE_CXXFLAGS += -msse2
#DEFINES += USE_BOOST=1

message("this says <foo & bar>")

maemo5 {
    target.path = /opt/usr/lib
    target.path = /opt
    INSTALLS += target
}

