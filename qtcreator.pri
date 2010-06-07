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
    return($$replace(1, /, $$QMAKE_DIR_SEP))
}

# For use in custom compilers which just copy files
win32:i_flag = i
defineReplace(stripSrcDir) {
    win32 {
        !contains(1, ^.:.*):1 = $$OUT_PWD/$$1
    } else {
        !contains(1, ^/.*):1 = $$OUT_PWD/$$1
    }
    out = $$cleanPath($$1)
    out ~= s|^$$re_escape($$PWD/)||$$i_flag
    return($$out)
}

isEmpty(TEST):CONFIG(debug, debug|release) {
    !debug_and_release|build_pass {
        TEST = 1
    }
}

isEmpty(IDE_LIBRARY_BASENAME) {
    IDE_LIBRARY_BASENAME = lib
}

DEFINES += IDE_LIBRARY_BASENAME=\\\"$$IDE_LIBRARY_BASENAME\\\"

equals(TEST, 1) {
    QT +=testlib
    DEFINES += WITH_TESTS
}

IDE_SOURCE_TREE = $$PWD
isEmpty(IDE_BUILD_TREE) {
    sub_dir = $$_PRO_FILE_PWD_
    sub_dir ~= s,^$$re_escape($$PWD),,
    IDE_BUILD_TREE = $$cleanPath($$OUT_PWD)
    IDE_BUILD_TREE ~= s,$$re_escape($$sub_dir)$,,
}
IDE_APP_PATH = $$IDE_BUILD_TREE/bin
macx {
    IDE_APP_TARGET   = "Qt Creator"
    IDE_LIBRARY_PATH = $$IDE_APP_PATH/$${IDE_APP_TARGET}.app/Contents/PlugIns
    IDE_PLUGIN_PATH  = $$IDE_LIBRARY_PATH
    IDE_LIBEXEC_PATH = $$IDE_APP_PATH/$${IDE_APP_TARGET}.app/Contents/Resources
    IDE_DATA_PATH    = $$IDE_APP_PATH/$${IDE_APP_TARGET}.app/Contents/Resources
    IDE_DOC_PATH     = $$IDE_DATA_PATH/doc
    contains(QT_CONFIG, ppc):CONFIG += ppc x86
    copydata = 1
} else {
    win32 {
        contains(TEMPLATE, vc.*)|contains(TEMPLATE_PREFIX, vc):vcproj = 1
        IDE_APP_TARGET   = qtcreator
    } else {
        IDE_APP_WRAPPER  = qtcreator
        IDE_APP_TARGET   = qtcreator.bin
    }
    IDE_LIBRARY_PATH = $$IDE_BUILD_TREE/$$IDE_LIBRARY_BASENAME/qtcreator
    IDE_PLUGIN_PATH  = $$IDE_LIBRARY_PATH/plugins
    IDE_LIBEXEC_PATH = $$IDE_APP_PATH # FIXME
    IDE_DATA_PATH    = $$IDE_BUILD_TREE/share/qtcreator
    IDE_DOC_PATH     = $$IDE_BUILD_TREE/share/doc/qtcreator
    !isEqual(IDE_SOURCE_TREE, $$IDE_BUILD_TREE):copydata = 1
}

INCLUDEPATH += \
    $$IDE_SOURCE_TREE/src/libs \
    $$IDE_SOURCE_TREE/tools

DEPENDPATH += \
    $$IDE_SOURCE_TREE/src/libs \
    $$IDE_SOURCE_TREE/tools

LIBS += -L$$IDE_LIBRARY_PATH

#DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
!macx:DEFINES += QT_USE_FAST_OPERATOR_PLUS QT_USE_FAST_CONCATENATION

unix {
    CONFIG(debug, debug|release):OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
    CONFIG(release, debug|release):OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared

    CONFIG(debug, debug|release):MOC_DIR = $${OUT_PWD}/.moc/debug-shared
    CONFIG(release, debug|release):MOC_DIR = $${OUT_PWD}/.moc/release-shared

    RCC_DIR = $${OUT_PWD}/.rcc
    UI_DIR = $${OUT_PWD}/.uic
}

linux-g++-* {
    # Bail out on non-selfcontained libraries. Just a security measure
    # to prevent checking in code that does not compile on other platforms.
    QMAKE_LFLAGS += -Wl,--allow-shlib-undefined -Wl,--no-undefined
}

win32-msvc* { 
    #Don't warn about sprintf, fopen etc being 'unsafe'
    DEFINES += _CRT_SECURE_NO_WARNINGS
}

# Handle S60 support: default on Windows, conditionally built on other platforms.
macx:SUPPORT_QT_S60 = $$(QTCREATOR_WITH_S60)
else:SUPPORT_QT_S60=1
