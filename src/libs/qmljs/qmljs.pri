INCLUDEPATH += \
    $$PWD/../../shared \
    $$PWD/../../shared/qmljs \
    $$PWD/../../shared/qmljs/parser

LIBS *= -l$$qtLibraryName(QmlJS)
DEFINES += QT_CREATOR
