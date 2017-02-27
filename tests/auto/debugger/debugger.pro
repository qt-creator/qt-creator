
TEMPLATE = subdirs

# Avoid race condition when compiling with multiple jobs or moc_predefs.h might get used and
# overridden at the same time as the sub projects are using the same build directory.
# Correct would be to completely rearrange these sub projects into sub directories, using the
# quick fix instead.
CONFIG += ordered

SUBDIRS += gdb.pro
SUBDIRS += simplifytypes.pro
SUBDIRS += dumpers.pro
SUBDIRS += namedemangler.pro
SUBDIRS += disassembler.pro
SUBDIRS += offsets.pro

