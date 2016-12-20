TEMPLATE = subdirs

# avoid race conditions when compiling shadowbuild and having more than one compile job
modeldemo.file = modeldemo.pro
testapps.depends = modeldemo

SUBDIRS += modeldemo testapps


