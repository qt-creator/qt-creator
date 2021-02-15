VPATH += $$PWD

# Input
HEADERS += itemlibraryview.h \
           $$PWD/itemlibraryiconimageprovider.h \
           itemlibrarywidget.h \
           itemlibrarymodel.h \
           itemlibraryresourceview.h \
           itemlibraryimageprovider.h \
           itemlibraryitem.h \
           itemlibrarycategory.h \
           itemlibraryitemsmodel.h \
           itemlibraryimport.h \
           itemlibrarycategoriesmodel.h \
           itemlibraryaddimportmodel.h \
           itemlibraryassetimportdialog.h \
           itemlibraryassetimporter.h \
           customfilesystemmodel.h \
           assetimportupdatedialog.h \
           assetimportupdatetreeitem.h \
           assetimportupdatetreeitemdelegate.h \
           assetimportupdatetreemodel.h \
           assetimportupdatetreeview.h

SOURCES += itemlibraryview.cpp \
           $$PWD/itemlibraryiconimageprovider.cpp \
           itemlibrarywidget.cpp \
           itemlibrarymodel.cpp \
           itemlibraryresourceview.cpp \
           itemlibraryimageprovider.cpp \
           itemlibraryitem.cpp \
           itemlibrarycategory.cpp \
           itemlibraryitemsmodel.cpp \
           itemlibraryimport.cpp \
           itemlibrarycategoriesmodel.cpp \
           itemlibraryaddimportmodel.cpp \
           itemlibraryassetimportdialog.cpp \
           itemlibraryassetimporter.cpp \
           customfilesystemmodel.cpp \
           assetimportupdatedialog.cpp \
           assetimportupdatetreeitem.cpp \
           assetimportupdatetreeitemdelegate.cpp \
           assetimportupdatetreemodel.cpp \
           assetimportupdatetreeview.cpp
RESOURCES += itemlibrary.qrc

FORMS += itemlibraryassetimportdialog.ui \
         assetimportupdatedialog.ui
