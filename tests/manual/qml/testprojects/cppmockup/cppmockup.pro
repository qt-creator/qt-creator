TEMPLATE = app

QT += qml quick
CONFIG += c++11

SOURCES += main.cpp \
    mybackendobject.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
### QML_IMPORT_PATH = $$PWD/mockups
QML_DESIGNER_IMPORT_PATH = $$PWD/mockups

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    mybackenmodel.h \
    mybackendobject.h
