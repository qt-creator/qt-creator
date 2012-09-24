include(../../../qtcreator.pri)

TARGET = qtcreator_crash_handler
DESTDIR = $$IDE_BIN_PATH

CONFIG -= app_bundle
TEMPLATE = app

SOURCES += \
    main.cpp \
    backtracecollector.cpp \
    crashhandlerdialog.cpp \
    crashhandler.cpp \
    utils.cpp \
    ../../libs/utils/checkablemessagebox.cpp \
    ../../libs/utils/environment.cpp


HEADERS += \
    backtracecollector.h \
    crashhandlerdialog.h \
    crashhandler.h \
    utils.h \
    ../../libs/utils/checkablemessagebox.h \
    ../../libs/utils/environment.h

FORMS += \
    crashhandlerdialog.ui

target.path = /bin
INSTALLS += target
