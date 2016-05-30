TEMPLATE=subdirs

SUBDIRS= \
    fakevim \
    debugger \
    subdir_proparser \
    shootout \
    pluginview

unix {
#   Uses popen
    SUBDIRS += \
#   Profile library paths issues
    process \
    ssh
}

subdir_proparser.file=proparser/testreader.pro
