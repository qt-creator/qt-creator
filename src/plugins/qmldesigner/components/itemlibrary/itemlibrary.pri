VPATH += $$PWD

# Input
HEADERS += itemlibraryview.h \
           itemlibrarywidget.h \
           itemlibrarymodel.h \
           itemlibrarytreeview.h \
           itemlibraryimageprovider.h \
           itemlibrarysectionmodel.h \
           itemlibraryitem.h \
           resourceitemdelegate.h \
           itemlibrarysection.h

SOURCES += itemlibraryview.cpp \
           itemlibrarywidget.cpp \
           itemlibrarymodel.cpp \
           itemlibrarytreeview.cpp \
           itemlibraryimageprovider.cpp \
           itemlibrarysectionmodel.cpp \
           itemlibraryitem.cpp \
           resourceitemdelegate.cpp \
           itemlibrarysection.cpp

RESOURCES += itemlibrary.qrc

OTHER_FILES += \
    qml/Selector.qml \
    qml/SectionView.qml \
    qml/Scrollbar.qml \
    qml/ItemView.qml \
    qml/ItemsViewStyle.qml \
    qml/ItemsView.qml
