VPATH += $$PWD

# Input
HEADERS += itemlibraryview.h itemlibrarywidget.h itemlibrarymodel.h itemlibrarycomponents.h itemlibraryimageprovider.h
SOURCES += itemlibraryview.cpp itemlibrarywidget.cpp itemlibrarymodel.cpp itemlibrarycomponents.cpp itemlibraryimageprovider.cpp
RESOURCES += itemlibrary.qrc

OTHER_FILES += \
    qml/Selector.qml \
    qml/SectionView.qml \
    qml/Scrollbar.qml \
    qml/ItemView.qml \
    qml/ItemsViewStyle.qml \
    qml/ItemsView.qml
