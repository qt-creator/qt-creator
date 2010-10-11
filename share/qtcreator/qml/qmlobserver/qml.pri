QT += declarative script network sql

contains(QT_CONFIG, opengl) {
    QT += opengl
    DEFINES += GL_SUPPORTED
}

!exists($$[QT_INSTALL_HEADERS]/QtCore/private/qabstractanimation_p.h) {
    DEFINES += NO_PRIVATE_HEADERS
}

INCLUDEPATH += $$PWD

HEADERS += $$PWD/qmlruntime.h \
           $$PWD/proxysettings.h \
           $$PWD/qdeclarativetester.h \
           $$PWD/deviceorientation.h \
           $$PWD/loggerwidget.h \
           $$PWD/crumblepath.h


SOURCES += $$PWD/qmlruntime.cpp \
           $$PWD/proxysettings.cpp \
           $$PWD/qdeclarativetester.cpp \
           $$PWD/loggerwidget.cpp \
           $$PWD/crumblepath.cpp

RESOURCES += $$PWD/qmlruntime.qrc \
    crumblepath.qrc

OTHER_FILES += toolbarstyle.css

maemo5 {
    QT += dbus
    HEADERS += $$PWD/texteditautoresizer_maemo5.h
    SOURCES += $$PWD/deviceorientation_maemo5.cpp
    FORMS += $$PWD/recopts_maemo5.ui \
            $$PWD/proxysettings_maemo5.ui
} else {
    SOURCES += $$PWD/deviceorientation.cpp
    FORMS += $$PWD/recopts.ui \
             $$PWD/proxysettings.ui
}
