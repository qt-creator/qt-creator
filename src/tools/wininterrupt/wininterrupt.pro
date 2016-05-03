CONFIG          += warn_on use_c_linker static
CONFIG          -= qt

include(../../qtcreatortool.pri)

# Switch to statically linked CRT. Note: There will be only one
# global state of the CRT, reconsider if other DLLs are required!
# TODO: No effect, currently?

msvc {
    QMAKE_CXXFLAGS_RELEASE    -= -MD
    QMAKE_CXXFLAGS_DEBUG      -= -MDd
    QMAKE_CXXFLAGS_RELEASE    += -MT
    QMAKE_CXXFLAGS_DEBUG      += -MT
} else {
    QMAKE_CXXFLAGS            += -static
}

SOURCES = wininterrupt.c

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

ENV_CPU=$$(CPU)
ENV_LIBPATH=$$(LIBPATH)
contains(ENV_CPU, ^AMD64$) {
    TARGET = win64interrupt
} else:isEmpty(ENV_CPU):contains(ENV_LIBPATH, ^.*amd64.*$) {
    TARGET = win64interrupt
} else {
    TARGET = win32interrupt
}
