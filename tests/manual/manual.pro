TEMPLATE=subdirs

SUBDIRS= \
cplusplus-frontend \
cplusplus-dump \
fakevim \
gdbdebugger \
preprocessor \
subdir_proparser \
trklauncher

win32 {
#   Uses CDB debugger
    SUBDIRS += ccdb
}

unix {
#   Uses popen
    SUBDIRS += \
    plain-cplusplus \
#   Profile library paths issues
    process \
    ssh
}

subdir_proparser.file=proparser/testreader.pro
