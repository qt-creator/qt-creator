TEMPLATE = subdirs

SUBDIRS = \
    bench \
    derived \
    dummy \
    gui

greaterThan(QT_MAJOR_VERSION, 4) {
    message("enabling quick tests")
    SUBDIRS += quickauto \
    quickauto2 \
    quickauto3
} else {
    message("quick tests disabled")
}

