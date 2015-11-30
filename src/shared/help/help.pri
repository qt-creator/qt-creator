VPATH += $$PWD

INCLUDEPATH *= $$PWD $$PWD/..
DEPENDPATH *= $$PWD $$PWD/..

# Input
HEADERS += \
        $$PWD/bookmarkmanager.h \
        $$PWD/contentwindow.h \
        $$PWD/filternamedialog.h \
        $$PWD/indexwindow.h \
        $$PWD/topicchooser.h \
        $$PWD/helpicons.h

SOURCES += \
        $$PWD/bookmarkmanager.cpp \
        $$PWD/contentwindow.cpp \
        $$PWD/filternamedialog.cpp \
        $$PWD/indexwindow.cpp \
        $$PWD/topicchooser.cpp

FORMS   += \
        $$PWD/bookmarkdialog.ui \
        $$PWD/filternamedialog.ui \
        $$PWD/topicchooser.ui
