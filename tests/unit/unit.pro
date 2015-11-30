TEMPLATE = subdirs

SUBDIRS = echoserver unittest

unittest.depends = echoserver
