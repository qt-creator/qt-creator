TEMPLATE = subdirs

SUBDIRS = qtpromaker \
     qmlpuppet

win32 {
    SUBDIRS += qtcdebugger
    !*g++*:SUBDIRS += mdnssd
    # win64interrupt only make sense for 64bit builds
    ENV_CPU=$$(CPU)
    ENV_LIBPATH=$$(LIBPATH)
    contains(ENV_CPU, ^AMD64$) {
        SUBDIRS += win64interrupt
    } else:isEmpty(ENV_CPU):contains(ENV_LIBPATH, ^.*amd64.*$) {
        SUBDIRS += win64interrupt
    }
} else {
    SUBDIRS += valgrindfake
}

QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    SUBDIRS += qtcrashhandler
}

