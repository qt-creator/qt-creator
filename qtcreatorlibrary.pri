IDE_SOURCE_TREE=$$(QTC_SOURCE)
IDE_BUILD_TREE=$$(QTC_BUILD)

isEmpty(IDE_SOURCE_TREE):error(Set QTC_SOURCE environment variable)
isEmpty(IDE_BUILD_TREE):error(Set QTC_BUILD environment variable)

include($$IDE_SOURCE_TREE/src/qtcreatorlibrary.pri)
