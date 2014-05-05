include(../shared/shared.pri)

# Inject the source dir for referencing test data from shadow builds.
DEFINES += SRCDIR=\\\"$$PWD\\\"

SOURCES += tst_c99.cpp
OTHER_FILES += \
    data/designatedInitializer.1.c
