TEMPLATE=subdirs

SUBDIRS= \
cplusplus-frontend \
fakevim \
debugger \
preprocessor \
subdir_proparser \
utils \
devices

unix {
#   Uses popen
    SUBDIRS += \
    plain-cplusplus \
#   Profile library paths issues
    process \
    ssh
}

subdir_proparser.file=proparser/testreader.pro
