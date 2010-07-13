TEMPLATE=subdirs

SUBDIRS= \
cplusplus \
cplusplus-dump \
fakevim \
gdbdebugger \
preprocessor \
subdir_proparser \
trklauncher

win {
#   Uses CDB debugger
    SUBDIRS += ccdb
}

unix {
#   Uses popen
    SUBDIRS += \
#   Profile library paths issues
    process \
    ssh
}

# Not compiling, popen + missing includes
# SUBDIRS += plain-cplusplus

subdir_proparser.file=proparser/testreader.pro
