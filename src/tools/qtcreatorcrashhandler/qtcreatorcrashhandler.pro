QTC_LIB_DEPENDS += utils

include(../../qtcreatortool.pri)

DESTDIR = $$IDE_BIN_PATH
TARGET = qtcreator_crash_handler

SOURCES += \
    main.cpp \
    backtracecollector.cpp \
    crashhandlerdialog.cpp \
    crashhandler.cpp \
    utils.cpp

HEADERS += \
    backtracecollector.h \
    crashhandlerdialog.h \
    crashhandler.h \
    utils.h

FORMS += \
    crashhandlerdialog.ui
