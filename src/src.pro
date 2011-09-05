TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = \
    libs/aggregation \ # needed by extensionsystem
    libs/extensionsystem \ # needed by app
    app \ # needed by libs/utils for app_version.h
    libs \
    plugins \
    tools
