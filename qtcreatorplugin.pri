IDE_SOURCE_TREE=$$(IDE_SOURCE_TREE)
IDE_BUILD_TREE=$$(IDE_BUILD_TREE)

isEmpty(IDE_SOURCE_TREE):error(Set IDE_SOURCE_TREE environment variable)
isEmpty(IDE_BUILD_TREE):error(Set IDE_BUILD_TREE environment variable)

include($$IDE_SOURCE_TREE/src/qtcreatorplugin.pri)
