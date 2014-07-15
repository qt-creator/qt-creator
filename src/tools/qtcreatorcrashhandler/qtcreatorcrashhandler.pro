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
    ../../libs/utils/qtcassert.cpp \
    ../../libs/utils/checkablemessagebox.cpp \
    ../../libs/utils/environment.cpp \
    ../../libs/utils/fileutils.cpp \
    ../../libs/utils/savefile.cpp


HEADERS += \
    backtracecollector.h \
    crashhandlerdialog.h \
    crashhandler.h \
    utils.h \
    ../../libs/utils/qtcassert.h \
    ../../libs/utils/checkablemessagebox.h \
    ../../libs/utils/environment.h \
    ../../libs/utils/fileutils.h \
    ../../libs/utils/savefile.h

FORMS += \
    crashhandlerdialog.ui

target.path = /bin
INSTALLS += target
