include(codemodelbackend.pri)

DESTDIR = $$IDE_LIBEXEC_PATH
target.path = $$QTC_PREFIX/bin # FIXME: libexec, more or less
INSTALLS += target
