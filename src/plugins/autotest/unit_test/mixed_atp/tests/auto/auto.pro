TEMPLATE = subdirs

SUBDIRS = \
    bench \
    dummy \
    gui

greaterThan(QT_MAJOR_VERSION, 4) {
    message("enabling quick tests")
    SUBDIRS += quickauto \
    quickauto2
} else {
    message("quick tests disabled")
}

