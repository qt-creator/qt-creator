TEMPLATE = subdirs

parsertests.file = parsertests.pro

# avoid race conditions when compiling shadowbuild and having more than one compile job
modeldemo.depends = parsertests
modeldemo.file = modeldemo.pro

testapps.depends = modeldemo parsertests

testrunner.file = testrunner.pro
testrunner.depends = testapps

SUBDIRS += parsertests modeldemo testapps testrunner


