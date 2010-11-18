TEMPLATE  = subdirs

# Build the Qt Creator CDB extension depending on whether
# CDB is actually detected.
# In order to do detection and building in one folder,
# use a subdir profile in '.'.

include(cdb_detect.pri)

!isEmpty(CDB_PATH) {
    SUBDIRS += lib_qtcreator_cdbext

    lib_qtcreator_cdbext.file = qtcreatorcdbext_build.pro
}
