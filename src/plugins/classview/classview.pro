include(../../qtcreatorplugin.pri)

HEADERS += \
    classviewplugin.h \
    classviewnavigationwidgetfactory.h \
    classviewconstants.h \
    classviewnavigationwidget.h \
    classviewparser.h \
    classviewmanager.h \
    classviewsymbollocation.h \
    classviewsymbolinformation.h \
    classviewparsertreeitem.h \
    classviewutils.h \
    classviewtreeitemmodel.h

SOURCES += \
    classviewplugin.cpp \
    classviewnavigationwidgetfactory.cpp \
    classviewnavigationwidget.cpp \
    classviewparser.cpp \
    classviewmanager.cpp \
    classviewsymbollocation.cpp \
    classviewsymbolinformation.cpp \
    classviewparsertreeitem.cpp \
    classviewutils.cpp \
    classviewtreeitemmodel.cpp

FORMS += \
    classviewnavigationwidget.ui

RESOURCES += \
    classview.qrc
