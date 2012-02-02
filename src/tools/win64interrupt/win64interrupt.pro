CONFIG          += warn_on console use_c_linker static
CONFIG          -= qt app_bundle

include(../../../qtcreator.pri)

SOURCES = win64interrupt.c

TEMPLATE        = app
DESTDIR         = $$IDE_LIBEXEC_PATH

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

target.path  = /bin # FIXME: libexec, more or less
INSTALLS    += target
