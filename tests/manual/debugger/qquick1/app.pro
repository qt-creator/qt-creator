QT += declarative

macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

SOURCES += main.cpp

OTHER_FILES += qml/main.qml

qml.files = qml
qml.path = .
DEPLOYMENT += qml
