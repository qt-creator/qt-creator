IDE_SOURCE_TREE = $$PWD/../

isEmpty(TEST) {
    CONFIG(debug, debug|release) {
        TEST = 1
    }
}

!isEmpty(TEST) {
    equals(TEST, 1) {
        QT +=testlib
        DEFINES+=WITH_TESTS
    }
}

isEmpty(IDE_BUILD_TREE) {
    error("qworkbench.pri: including file must define IDE_BUILD_TREE (probably a relative path)")
}
macx {
    IDE_APP_TARGET = QtCreator
    IDE_LIBRARY_PATH = $$IDE_BUILD_TREE/bin/$${IDE_APP_TARGET}.app/Contents/PlugIns
    contains(QT_CONFIG, ppc):CONFIG += ppc x86
} else {
    IDE_APP_TARGET = qtcreator
    IDE_LIBRARY_PATH = $$IDE_BUILD_TREE/lib
}
IDE_APP_PATH = $$IDE_BUILD_TREE/bin
win32 {
    IDE_LIBRARY_PATH ~= s|/+|\|
    IDE_APP_PATH ~= s|/+|\|
}

INCLUDEPATH += \
    $$IDE_SOURCE_TREE/src/libs \
    $$IDE_SOURCE_TREE/tools \

DEPENDPATH += \
    $$IDE_SOURCE_TREE/src/libs \
    $$IDE_SOURCE_TREE/tools \

LIBS += -L$$IDE_LIBRARY_PATH
