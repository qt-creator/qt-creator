TEMPLATE = subdirs

parsertests.file = parsertests.pro

modeldemo.file = modeldemo.pro

testapps.depends = modeldemo parsertests

testrunner.file = testrunner.pro
testrunner.depends = testapps

SUBDIRS += parsertests modeldemo testapps testrunner


