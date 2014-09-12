QT += qml quick

macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

SOURCES += main.cpp

qml.files = qml
qml.path = .
DEPLOYMENT += qml
