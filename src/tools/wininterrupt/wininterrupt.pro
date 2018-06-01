CONFIG          += warn_on use_c_linker static
CONFIG          -= qt

include(../../qtcreatortool.pri)

# Switch to statically linked CRT. Note: There will be only one
# global state of the CRT, reconsider if other DLLs are required!
# TODO: No effect, currently?

msvc {
    QMAKE_CFLAGS_RELEASE    -= -MD
    QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO -= -MD
    QMAKE_CFLAGS_DEBUG      -= -MDd
    QMAKE_CFLAGS_RELEASE    += -MT
    QMAKE_CFLAGS_DEBUG      += -MT
    QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO += -MT
} else {
    QMAKE_CFLAGS            += -static
}

SOURCES = wininterrupt.c

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

# Check for VSCMD_ARG_TGT_ARCH (VS 17) or Platform=X64 (VS 13, 15)
# For older versions, fall back to hacky check on LIBPATH
ENV_TARGET_ARCH=$$(VSCMD_ARG_TGT_ARCH)
isEmpty(ENV_TARGET_ARCH):ENV_TARGET_ARCH = $$(Platform)
ENV_LIBPATH=$$(LIBPATH)
contains(ENV_TARGET_ARCH, .*64$) {
    TARGET = win64interrupt
} else:isEmpty(ENV_TARGET_ARCH):contains(ENV_LIBPATH, ^.*amd64.*$) {
    TARGET = win64interrupt
} else {
    TARGET = win32interrupt
}
