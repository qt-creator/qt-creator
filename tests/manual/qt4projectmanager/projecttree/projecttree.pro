#-------------------------------------------------
#
# Project created by QtCreator 2009-05-12T12:31:37
#
#-------------------------------------------------

TARGET = projecttree
TEMPLATE = app

include(prifile/prifile.pri)

SOURCES += main.cpp\
        widget.cpp \
        /etc/cups/cupsd.conf \
        /etc/cups/printers.conf \
        /etc/apache2/mime.types \
        /etc/apache2/extra/httpd-info.conf \
        ../projecttree_data1/a/foo.cpp \
        ../projecttree_data1/b/foo.cpp \
        ../projecttree_data2/a/bar.cpp \
        ../projecttree_data2/a/sub/bar2.cpp \
        ../projecttree_data3/path/bar.cpp \
        subpath/a/foo.cpp \
        subpath/b/foo.cpp \
        sub2/a/bar.cpp \
        sub2/a/sub/bar2.cpp \
        uniquesub/path/bar.cpp \
        sources/foo.cpp \
        sources/bar.cpp

HEADERS += main.h\
        widget.h \
        ../projecttree_data1/a/foo.h \
        ../projecttree_data1/b/foo.h \
        ../projecttree_data2/a/bar.h \
        ../projecttree_data2/a/sub/bar2.h \
        ../projecttree_data3/path/bar.h \
        subpath/a/foo.h \
        subpath/b/foo.h \
        sub2/a/bar.h \
        sub2/a/sub/bar2.h \
        uniquesub/path/bar.h \
        headers/foo.h \
        headers/bar.h

FORMS    += widget.ui
RESOURCES += resource.qrc
DISTFILES += foo.txt
