TEMPLATE = subdirs

SUBDIRS += lib \
           tests

tests.depends = lib
