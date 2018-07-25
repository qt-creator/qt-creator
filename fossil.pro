isEmpty(IDE_SOURCE_TREE): IDE_SOURCE_TREE = $$(QTC_SOURCE)
isEmpty(IDE_SOURCE_TREE): error("You need to set the environment variable QTC_SOURCE to point to the directory where the Qt Creator sources are")

isEmpty(IDE_BUILD_TREE): IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE): error("You need to set the environment variable QTC_BUILD to point to the directory where Qt Creator was built")

TEMPLATE = subdirs
SUBDIRS += plugins/fossil

include($$IDE_SOURCE_TREE/qtcreator.pri)
include(doc/doc.pri)
