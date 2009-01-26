VPATH += $$PWD

INCLUDEPATH *= $$PWD $$PWD/..

# Input
HEADERS += \
        $$PWD/filternamedialog.h \
        $$PWD/topicchooser.h \
        $$PWD/helpviewer.h \
        $$PWD/contentwindow.h \
        $$PWD/bookmarkmanager.h \
        $$PWD/../namespace_global.h \
        $$PWD/indexwindow.h

SOURCES += \
        $$PWD/filternamedialog.cpp \
        $$PWD/topicchooser.cpp \
        $$PWD/helpviewer.cpp \
        $$PWD/indexwindow.cpp \
        $$PWD/contentwindow.cpp \
        $$PWD/bookmarkmanager.cpp

FORMS   += \
        $$PWD/filternamedialog.ui \
        $$PWD/topicchooser.ui \
        $$PWD/bookmarkdialog.ui
