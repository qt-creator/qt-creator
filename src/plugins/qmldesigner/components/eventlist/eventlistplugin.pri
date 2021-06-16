QT *= qml quick core

VPATH += $$PWD

SOURCES += $$PWD/eventlist.cpp \
           $$PWD/eventlistview.cpp \
           $$PWD/eventlistpluginview.cpp \
           $$PWD/eventlistactions.cpp \
           $$PWD/eventlistdialog.cpp \
           $$PWD/eventlistdelegate.cpp \
           $$PWD/filterlinewidget.cpp \
           $$PWD/nodelistview.cpp \
           $$PWD/nodelistdelegate.cpp \
           $$PWD/nodeselectionmodel.cpp \
           $$PWD/assigneventdialog.cpp \
           $$PWD/shortcutwidget.cpp \
           $$PWD/connectsignaldialog.cpp \
           $$PWD/eventlistutils.cpp

HEADERS += $$PWD/eventlist.h \
           $$PWD/eventlistpluginview.h \
           $$PWD/eventlistview.h \
           $$PWD/eventlistactions.h \
           $$PWD/eventlistdialog.h \
           $$PWD/eventlistdelegate.h \
           $$PWD/filterlinewidget.h \
           $$PWD/nodelistview.h \
           $$PWD/nodelistdelegate.h \
           $$PWD/nodeselectionmodel.h \
           $$PWD/assigneventdialog.h \
           $$PWD/shortcutwidget.h \
           $$PWD/connectsignaldialog.h \
           $$PWD/eventlistutils.h

RESOURCES += $$PWD/eventlistplugin.qrc
