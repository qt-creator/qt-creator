INCLUDEPATH += $$PWD/

minQtVersion(5, 6, 0) {
HEADERS += \
    $$PWD/qmlprivategate.h

SOURCES += \
    $$PWD/qmlprivategate_56.cpp

} else {

HEADERS += \
    $$PWD/qmlprivategate.h \
    $$PWD/metaobject.h \
    $$PWD/designercustomobjectdata.h

SOURCES += \
    $$PWD/qmlprivategate.cpp \
    $$PWD/metaobject.cpp \
    $$PWD/designercustomobjectdata.cpp
}
