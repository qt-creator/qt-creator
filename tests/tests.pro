TEMPLATE=subdirs
SUBDIRS += auto manual tools
!win32 {
    SUBDIRS += valgrind
}
