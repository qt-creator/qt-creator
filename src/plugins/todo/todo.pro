include(../../qtcreatorplugin.pri)

HEADERS += todoplugin.h \
    keyword.h \
    constants.h \
    todooutputpane.h \
    todoitem.h \
    settings.h \
    optionspage.h \
    optionsdialog.h\
    keyworddialog.h \
    todoitemsprovider.h \
    todoitemsmodel.h \
    todoitemsscanner.h \
    cpptodoitemsscanner.h \
    qmljstodoitemsscanner.h \
    lineparser.h \
    todooutputtreeview.h \
    todooutputtreeviewdelegate.h \
    todoprojectsettingswidget.h \
    todoicons.h

SOURCES += todoplugin.cpp \
    keyword.cpp \
    todooutputpane.cpp \
    optionspage.cpp \
    settings.cpp \
    optionsdialog.cpp \
    keyworddialog.cpp \
    todoitemsprovider.cpp \
    todoitemsmodel.cpp \
    todoitemsscanner.cpp \
    cpptodoitemsscanner.cpp \
    qmljstodoitemsscanner.cpp \
    lineparser.cpp \
    todooutputtreeview.cpp \
    todooutputtreeviewdelegate.cpp \
    todoprojectsettingswidget.cpp \
    todoicons.cpp

RESOURCES += \
    todoplugin.qrc

FORMS += \
    optionsdialog.ui \
    keyworddialog.ui \
    todoprojectsettingswidget.ui
