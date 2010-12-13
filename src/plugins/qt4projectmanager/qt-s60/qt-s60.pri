!isEmpty(SUPPORT_QT_S60) {
    DEFINES += QTCREATOR_WITH_S60
}
SOURCES += $$PWD/s60devices.cpp \
    $$PWD/s60devicespreferencepane.cpp \
    $$PWD/s60manager.cpp \
    $$PWD/sbsv2parser.cpp \
    $$PWD/winscwtoolchain.cpp \
    $$PWD/gccetoolchain.cpp \
    $$PWD/s60emulatorrunconfiguration.cpp \
    $$PWD/s60devicerunconfiguration.cpp \
    $$PWD/s60devicerunconfigurationwidget.cpp \
    $$PWD/s60projectchecker.cpp \
    $$PWD/rvcttoolchain.cpp \
    $$PWD/s60runconfigbluetoothstarter.cpp \
    $$PWD/abldparser.cpp \
    $$PWD/rvctparser.cpp \
    $$PWD/winscwparser.cpp \
    $$PWD/s60createpackagestep.cpp \
    $$PWD/s60deploystep.cpp \
    $$PWD/s60createpackageparser.cpp \
    $$PWD/passphraseforkeydialog.cpp \
    $$PWD/s60deployconfiguration.cpp \
    $$PWD/s60deployconfigurationwidget.cpp \
    $$PWD/s60certificateinfo.cpp \
    $$PWD/certificatepathchooser.cpp \
    $$PWD/s60symbiancertificate.cpp \
    $$PWD/s60certificatedetailsdialog.cpp
HEADERS += $$PWD/s60devices.h \
    $$PWD/s60devicespreferencepane.h \
    $$PWD/s60manager.h \
    $$PWD/sbsv2parser.h \
    $$PWD/winscwtoolchain.h \
    $$PWD/gccetoolchain.h \
    $$PWD/s60emulatorrunconfiguration.h \
    $$PWD/s60devicerunconfiguration.h \
    $$PWD/s60devicerunconfigurationwidget.h \
    $$PWD/s60projectchecker.h \
    $$PWD/rvcttoolchain.h \
    $$PWD/s60runconfigbluetoothstarter.h \
    $$PWD/abldparser.h \
    $$PWD/rvctparser.h \
    $$PWD/winscwparser.h \
    $$PWD/s60createpackagestep.h \
    $$PWD/s60deploystep.h \
    $$PWD/s60createpackageparser.h \
    $$PWD/passphraseforkeydialog.h \
    $$PWD/s60deployconfiguration.h \
    $$PWD/s60deployconfigurationwidget.h \
    $$PWD/s60certificateinfo.h \
    $$PWD/certificatepathchooser.h \
    $$PWD/s60symbiancertificate.h \
    $$PWD/s60certificatedetailsdialog.h
FORMS += $$PWD/s60devicespreferencepane.ui \
    $$PWD/s60createpackagestep.ui \
    $$PWD/s60certificatedetailsdialog.ui
