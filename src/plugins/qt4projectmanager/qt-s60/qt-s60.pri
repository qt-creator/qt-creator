SUPPORT_QT_S60 = $$(QTCREATOR_WITH_S60)
!isEmpty(SUPPORT_QT_S60) {
    message("Adding experimental support for Qt/S60 applications.")
    DEFINES += QTCREATOR_WITH_S60

    SOURCES += $$PWD/s60devices.cpp \
        $$PWD/s60devicespreferencepane.cpp \
        $$PWD/s60manager.cpp

    HEADERS += $$PWD/s60devices.h \
        $$PWD/s60devicespreferencepane.h \
        $$PWD/s60manager.h

    FORMS   += $$PWD/s60devicespreferencepane.ui
}
