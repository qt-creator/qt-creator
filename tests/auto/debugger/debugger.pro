
TEMPLATE = subdirs

SUBDIRS += gdb.pro
SUBDIRS += dumpers.pro
SUBDIRS += namedemangler.pro
SUBDIRS += simplifytypes.pro
SUBDIRS += disassembler.pro
greaterThan(QT_MAJOR_VERSION, 4):greaterThan(QT_MINOR_VERSION, 1) | lessThan(QT_MAJOR_VERSION, 5) {
    SUBDIRS += offsets.pro
}

