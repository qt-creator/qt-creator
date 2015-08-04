isEmpty(IDE_SOURCE_TREE): IDE_SOURCE_TREE = $$(QTC_SOURCE)
isEmpty(IDE_BUILD_TREE): IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_SOURCE_TREE): error("Set QTC_SOURCE environment variable")
isEmpty(IDE_BUILD_TREE): error("Set QTC_BUILD environment variable")

TEMPLATE = aux

STATIC_BASE = $$PWD
STATIC_DIRS = templates

for(data_dir, STATIC_DIRS) {
    files = $$files($$STATIC_BASE/$$data_dir/*, true)
    for(file, files): !exists($$file/*): STATIC_FILES += $$file
}

include($$IDE_SOURCE_TREE/share/qtcreator/static.pri)

