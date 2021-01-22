VPATH += $$PWD

# Input
HEADERS += itemlibraryview.h \
           $$PWD/itemlibraryiconimageprovider.h \
           itemlibrarywidget.h \
           itemlibrarymodel.h \
           itemlibraryresourceview.h \
           itemlibraryimageprovider.h \
           itemlibrarysectionmodel.h \
           itemlibraryitem.h \
           itemlibrarysection.h \
           itemlibraryassetimportdialog.h \
           itemlibraryassetimporter.h \
           customfilesystemmodel.h

SOURCES += itemlibraryview.cpp \
           $$PWD/itemlibraryiconimageprovider.cpp \
           itemlibrarywidget.cpp \
           itemlibrarymodel.cpp \
           itemlibraryresourceview.cpp \
           itemlibraryimageprovider.cpp \
           itemlibrarysectionmodel.cpp \
           itemlibraryitem.cpp \
           itemlibrarysection.cpp \
           itemlibraryassetimportdialog.cpp \
           itemlibraryassetimporter.cpp \
           customfilesystemmodel.cpp
RESOURCES += itemlibrary.qrc

FORMS += itemlibraryassetimportdialog.ui
