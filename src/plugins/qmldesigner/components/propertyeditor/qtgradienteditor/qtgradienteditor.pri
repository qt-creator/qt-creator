INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
FORMS +=     $$PWD/qtgradienteditor.ui \
             $$PWD/qtgradientdialog.ui \
             $$PWD/qtgradientview.ui \
             $$PWD/qtgradientviewdialog.ui
SOURCES +=   $$PWD/qtgradientstopsmodel.cpp \
             $$PWD/qtgradientstopswidget.cpp \
             $$PWD/qtgradientstopscontroller.cpp \
             $$PWD/qtgradientwidget.cpp \
             $$PWD/qtgradienteditor.cpp \
             $$PWD/qtgradientdialog.cpp \
             $$PWD/qtcolorbutton.cpp \
             $$PWD/qtcolorline.cpp \
             $$PWD/qtgradientview.cpp \
             $$PWD/qtgradientviewdialog.cpp \
             $$PWD/qtgradientmanager.cpp \
             $$PWD/qtgradientutils.cpp
HEADERS +=   $$PWD/qtgradientstopsmodel.h \
             $$PWD/qtgradientstopswidget.h \
             $$PWD/qtgradientstopscontroller.h \
             $$PWD/qtgradientwidget.h \
             $$PWD/qtgradienteditor.h \
             $$PWD/qtgradientdialog.h \
             $$PWD/qtcolorbutton.h \
             $$PWD/qtcolorline.h \
             $$PWD/qtgradientview.h \
             $$PWD/qtgradientviewdialog.h \
             $$PWD/qtgradientmanager.h \
             $$PWD/qtgradientutils.h
RESOURCES += $$PWD/qtgradienteditor.qrc

QT += xml
