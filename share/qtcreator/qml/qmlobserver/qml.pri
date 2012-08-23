QT += declarative script network sql
contains(QT_CONFIG, opengl) {
    QT += opengl
    DEFINES += GL_SUPPORTED
}

INCLUDEPATH += $$PWD

HEADERS += $$PWD/qmlruntime.h \
           $$PWD/proxysettings.h \
           $$PWD/qdeclarativetester.h \
           $$PWD/deviceorientation.h \
           $$PWD/loggerwidget.h
SOURCES += $$PWD/qmlruntime.cpp \
           $$PWD/proxysettings.cpp \
           $$PWD/qdeclarativetester.cpp \
           $$PWD/loggerwidget.cpp

RESOURCES = $$PWD/browser/browser.qrc \
            $$PWD/startup/startup.qrc

maemo5 {
    QT += dbus
    HEADERS += $$PWD/texteditautoresizer_maemo5.h
    SOURCES += $$PWD/deviceorientation_maemo5.cpp
    FORMS = $$PWD/recopts_maemo5.ui \
            $$PWD/proxysettings_maemo5.ui
} else:linux-g++-maemo {
    QT += dbus
    SOURCES += $$PWD/deviceorientation_harmattan.cpp
    FORMS = $$PWD/recopts.ui \
            $$PWD/proxysettings.ui
} else {
    SOURCES += $$PWD/deviceorientation.cpp
    FORMS = $$PWD/recopts.ui \
            $$PWD/proxysettings.ui
}
