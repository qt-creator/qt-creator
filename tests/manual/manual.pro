TEMPLATE=subdirs

SUBDIRS= \
cplusplus-frontend \
fakevim \
debugger \
subdir_proparser \
utils

unix {
#   Uses popen
    SUBDIRS += \
#   Profile library paths issues
    process \
    ssh
}

subdir_proparser.file=proparser/testreader.pro
