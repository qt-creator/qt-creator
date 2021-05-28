VPATH += $$PWD

# Input
HEADERS += itemlibraryview.h \
           $$PWD/itemlibraryiconimageprovider.h \
           itemlibrarywidget.h \
           itemlibrarymodel.h \
           itemlibraryimageprovider.h \
           itemlibraryitem.h \
           itemlibrarycategory.h \
           itemlibraryitemsmodel.h \
           itemlibraryimport.h \
           itemlibrarycategoriesmodel.h \
           itemlibraryaddimportmodel.h \
           itemlibraryassetimportdialog.h \
           itemlibraryassetimporter.h \
           itemlibraryassetsdir.h \
           itemlibraryassetsdirsmodel.h \
           itemlibraryassetsfilesmodel.h \
           itemlibraryassetsiconprovider.h \
           itemlibraryassetsmodel.h \
           assetimportupdatedialog.h \
           assetimportupdatetreeitem.h \
           assetimportupdatetreeitemdelegate.h \
           assetimportupdatetreemodel.h \
           assetimportupdatetreeview.h

SOURCES += itemlibraryview.cpp \
           $$PWD/itemlibraryiconimageprovider.cpp \
           itemlibrarywidget.cpp \
           itemlibrarymodel.cpp \
           itemlibraryimageprovider.cpp \
           itemlibraryitem.cpp \
           itemlibrarycategory.cpp \
           itemlibraryitemsmodel.cpp \
           itemlibraryimport.cpp \
           itemlibrarycategoriesmodel.cpp \
           itemlibraryaddimportmodel.cpp \
           itemlibraryassetimportdialog.cpp \
           itemlibraryassetimporter.cpp \
           itemlibraryassetsdir.cpp \
           itemlibraryassetsdirsmodel.cpp \
           itemlibraryassetsfilesmodel.cpp \
           itemlibraryassetsiconprovider.cpp \
           itemlibraryassetsmodel.cpp \
           assetimportupdatedialog.cpp \
           assetimportupdatetreeitem.cpp \
           assetimportupdatetreeitemdelegate.cpp \
           assetimportupdatetreemodel.cpp \
           assetimportupdatetreeview.cpp
RESOURCES += itemlibrary.qrc

FORMS += itemlibraryassetimportdialog.ui \
         assetimportupdatedialog.ui
