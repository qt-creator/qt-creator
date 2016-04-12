TEMPLATE = subdirs

mac {
    SUBDIRS += \
    iossim \
    iossim_1_8_2
}

isEmpty(BUILD_CPLUSPLUS_TOOLS):BUILD_CPLUSPLUS_TOOLS=$$(BUILD_CPLUSPLUS_TOOLS)
!isEmpty(BUILD_CPLUSPLUS_TOOLS): SUBDIRS += cplusplus-keywordgen
