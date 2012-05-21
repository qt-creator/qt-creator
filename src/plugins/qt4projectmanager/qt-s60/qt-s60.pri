CONFIG += exceptions

SOURCES += $$PWD/s60manager.cpp \
    $$PWD/symbianidevice.cpp \
    $$PWD/symbianideviceconfigwidget.cpp \
    $$PWD/symbianidevicefactory.cpp \
    $$PWD/sbsv2parser.cpp \
    $$PWD/gccetoolchain.cpp \
    $$PWD/s60devicerunconfiguration.cpp \
    $$PWD/s60devicerunconfigurationwidget.cpp \
    $$PWD/rvcttoolchain.cpp \
    $$PWD/abldparser.cpp \
    $$PWD/rvctparser.cpp \
    $$PWD/s60createpackagestep.cpp \
    $$PWD/s60deploystep.cpp \
    $$PWD/s60createpackageparser.cpp \
    $$PWD/passphraseforkeydialog.cpp \
    $$PWD/s60deployconfiguration.cpp \
    $$PWD/s60deployconfigurationwidget.cpp \
    $$PWD/s60certificateinfo.cpp \
    $$PWD/certificatepathchooser.cpp \
    $$PWD/s60symbiancertificate.cpp \
    $$PWD/s60certificatedetailsdialog.cpp \
    $$PWD/qt4symbiantargetfactory.cpp \
    $$PWD/qt4symbiantarget.cpp \
    $$PWD/s60runcontrolfactory.cpp \
    $$PWD/codaruncontrol.cpp \
    $$PWD/s60runcontrolbase.cpp \
    $$PWD/s60publishingwizardfactories.cpp \
    $$PWD/s60publishingwizardovi.cpp \
    $$PWD/s60publishingsissettingspageovi.cpp \
    $$PWD/s60publisherovi.cpp \
    $$PWD/s60publishingbuildsettingspageovi.cpp \
    $$PWD/s60publishingresultspageovi.cpp \
    $$PWD/symbianqtversionfactory.cpp \
    $$PWD/symbianqtversion.cpp \
    $$PWD/s60devicedebugruncontrol.cpp

HEADERS += $$PWD/s60manager.h \
    $$PWD/symbianidevice.h \
    $$PWD/symbianideviceconfigwidget.h \
    $$PWD/symbianidevicefactory.h \
    $$PWD/sbsv2parser.h \
    $$PWD/gccetoolchain.h \
    $$PWD/s60devicerunconfiguration.h \
    $$PWD/s60devicerunconfigurationwidget.h \
    $$PWD/rvcttoolchain.h \
    $$PWD/abldparser.h \
    $$PWD/rvctparser.h \
    $$PWD/s60createpackagestep.h \
    $$PWD/s60deploystep.h \
    $$PWD/s60createpackageparser.h \
    $$PWD/passphraseforkeydialog.h \
    $$PWD/s60deployconfiguration.h \
    $$PWD/s60deployconfigurationwidget.h \
    $$PWD/s60certificateinfo.h \
    $$PWD/certificatepathchooser.h \
    $$PWD/s60symbiancertificate.h \
    $$PWD/s60certificatedetailsdialog.h \
    $$PWD/qt4symbiantargetfactory.h \
    $$PWD/qt4symbiantarget.h \
    $$PWD/s60runcontrolfactory.h \
    $$PWD/codaruncontrol.h \
    $$PWD/s60runcontrolbase.h \
    $$PWD/s60publishingwizardfactories.h \
    $$PWD/s60publishingwizardovi.h \
    $$PWD/s60publishingsissettingspageovi.h \
    $$PWD/s60publisherovi.h \
    $$PWD/s60publishingbuildsettingspageovi.h \
    $$PWD/s60publishingresultspageovi.h \
    $$PWD/symbianqtversionfactory.h \
    $$PWD/symbianqtversion.h \
    $$PWD/s60devicedebugruncontrol.h

FORMS += $$PWD/s60createpackagestep.ui \
    $$PWD/s60certificatedetailsdialog.ui \
    $$PWD/rvcttoolchainconfigwidget.ui \
    $$PWD/s60publishingbuildsettingspageovi.ui \
    $$PWD/s60publishingresultspageovi.ui \
    $$PWD/s60publishingsissettingspageovi.ui

include(../../../shared/json/json.pri)
DEFINES += JSON_INCLUDE_PRI
