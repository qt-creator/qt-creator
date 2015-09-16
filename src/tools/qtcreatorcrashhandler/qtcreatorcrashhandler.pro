QTC_LIB_DEPENDS += utils

include(../../qtcreatortool.pri)

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
