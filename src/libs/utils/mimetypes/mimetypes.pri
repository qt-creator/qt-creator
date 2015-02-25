# Qt core mimetype module

HEADERS += \
        $$PWD/mimedatabase.h \
        $$PWD/mimetype.h \
        $$PWD/mimemagicrulematcher_p.h \
        $$PWD/mimetype_p.h \
        $$PWD/mimetypeparser_p.h \
        $$PWD/mimedatabase_p.h \
        $$PWD/mimemagicrule_p.h \
        $$PWD/mimeglobpattern_p.h \
        $$PWD/mimeprovider_p.h

SOURCES += \
        $$PWD/mimedatabase.cpp \
        $$PWD/mimetype.cpp \
        $$PWD/mimemagicrulematcher.cpp \
        $$PWD/mimetypeparser.cpp \
        $$PWD/mimemagicrule.cpp \
        $$PWD/mimeglobpattern.cpp \
        $$PWD/mimeprovider.cpp

OTHER_FILES += $$[QT_HOST_DATA/src]/src/corelib/mimetypes/mime/packages/freedesktop.org.xml
