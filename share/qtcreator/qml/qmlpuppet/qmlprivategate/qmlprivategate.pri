INCLUDEPATH += $$PWD/

# in case we are building the puppet inside qtcreator we don't have the qtcreator.pri where this function comes from
defineTest(minQtVersion) {
    maj = $$1
    min = $$2
    patch = $$3
    isEqual(QT_MAJOR_VERSION, $$maj) {
        isEqual(QT_MINOR_VERSION, $$min) {
            isEqual(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
            greaterThan(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
        }
        greaterThan(QT_MINOR_VERSION, $$min) {
            return(true)
        }
    }
    greaterThan(QT_MAJOR_VERSION, $$maj) {
        return(true)
    }
    return(false)
}

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
