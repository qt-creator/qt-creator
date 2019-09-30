VPATH += $$PWD

qtHaveModule(quick3dassetimport) {
    QT *= quick3dassetimport-private
    DEFINES *= IMPORT_QUICK3D_ASSETS
}

# Input
HEADERS += itemlibraryview.h \
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
