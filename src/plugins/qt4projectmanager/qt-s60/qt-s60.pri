win32:SUPPORT_QT_S60=1
else:SUPPORT_QT_S60 = $$(QTCREATOR_WITH_S60)
!isEmpty(SUPPORT_QT_S60) { 
    message("Adding experimental support for Qt/S60 applications.")
    DEFINES += QTCREATOR_WITH_S60
    SOURCES += $$PWD/s60devices.cpp \
        $$PWD/s60devicespreferencepane.cpp \
        $$PWD/s60manager.cpp \
        $$PWD/winscwtoolchain.cpp \
        $$PWD/gccetoolchain.cpp \
        $$PWD/s60emulatorrunconfiguration.cpp \
        $$PWD/s60devicerunconfiguration.cpp \
        $$PWD/serialdevicelister.cpp \
        $$PWD/rvcttoolchain.cpp
    HEADERS += $$PWD/s60devices.h \
        $$PWD/s60devicespreferencepane.h \
        $$PWD/s60manager.h \
        $$PWD/winscwtoolchain.h \
        $$PWD/gccetoolchain.h \
        $$PWD/s60emulatorrunconfiguration.h \
        $$PWD/s60devicerunconfiguration.h \
        $$PWD/serialdevicelister.h \
        $$PWD/rvcttoolchain.h
    FORMS += $$PWD/s60devicespreferencepane.ui
    OTHER_FILES += $$PWD/qt-s60-todo.txt
    include(../../../shared/trk/trk.pri)||error("could not include trk.pri")
}
