defineReplace(cleanPath) {
    win32:1 ~= s|\\\\|/|g
    contains(1, ^/.*):pfx = /
    else:pfx =
    segs = $$split(1, /)
    out =
    for(seg, segs) {
        equals(seg, ..):out = $$member(out, 0, -2)
        else:!equals(seg, .):out += $$seg
    }
    return($$join(out, /, $$pfx))
}

defineReplace(targetPath) {
    win32:1 ~= s|/|\|g
    return($$1)
}

isEmpty(TEST):CONFIG(debug, debug|release) {
    !debug_and_release|build_pass {
        TEST = 1
    }
}

linux-*-64 {
    IDE_LIBRARY_BASENAME = lib64
} else {
    IDE_LIBRARY_BASENAME = lib
}

equals(TEST, 1) {
    QT +=testlib
    DEFINES += WITH_TESTS
}

IDE_SOURCE_TREE = $$PWD
sub_dir = $$_PRO_FILE_PWD_
sub_dir ~= s,^$$re_escape($$PWD),,
IDE_BUILD_TREE = $$cleanPath($$OUT_PWD)
IDE_BUILD_TREE ~= s,$$re_escape($$sub_dir)$,,
macx {
    IDE_APP_TARGET   = QtCreator
    IDE_LIBRARY_PATH = $$IDE_BUILD_TREE/bin/$${IDE_APP_TARGET}.app/Contents/PlugIns
    IDE_PLUGIN_PATH  = $$IDE_LIBRARY_PATH
    IDE_DATA_PATH    = $$IDE_BUILD_TREE/bin/$${IDE_APP_TARGET}.app/Contents/Resources
    contains(QT_CONFIG, ppc):CONFIG += ppc x86
} else {
    win32 {
        IDE_APP_TARGET   = qtcreator
    } else {
        IDE_APP_WRAPPER  = qtcreator
        IDE_APP_TARGET   = qtcreator.bin
    }
    IDE_LIBRARY_PATH = $$IDE_BUILD_TREE/$$IDE_LIBRARY_BASENAME/qtcreator
    IDE_PLUGIN_PATH  = $$IDE_LIBRARY_PATH/plugins
    IDE_DATA_PATH    = $$IDE_BUILD_TREE/share/qtcreator
}
IDE_APP_PATH = $$IDE_BUILD_TREE/bin

INCLUDEPATH += \
    $$IDE_SOURCE_TREE/src/libs \
    $$IDE_SOURCE_TREE/tools

DEPENDPATH += \
    $$IDE_SOURCE_TREE/src/libs \
    $$IDE_SOURCE_TREE/tools

LIBS += -L$$IDE_LIBRARY_PATH

# DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII

unix {
    debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
    release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared

    debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
    release:MOC_DIR = $${OUT_PWD}/.moc/release-shared

    RCC_DIR = $${OUT_PWD}/.rcc
    UI_DIR = $${OUT_PWD}/.uic
}

linux-g++-* {
    # Bail out on non-selfcontained libraries. Just a security measure
    # to prevent checking in code that does not compile on other platforms.
    QMAKE_LFLAGS += -Wl,--allow-shlib-undefined -Wl,--no-undefined
}
