TEMPLATE=subdirs

SUBDIRS= \
fakevim \
debugger \
subdir_proparser \
utils \
shootout

unix {
#   Uses popen
    SUBDIRS += \
#   Profile library paths issues
    process \
    ssh
}

subdir_proparser.file=proparser/testreader.pro
