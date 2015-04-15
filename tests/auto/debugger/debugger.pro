
TEMPLATE = subdirs

gdb.file = gdb.pro
simplifytypes.file = simplifytypes.pro

# avoid race condition when compiling with multiple jobs
dumpers.depends = gdb simplifytypes
dumpers.file = dumpers.pro

SUBDIRS += gdb
SUBDIRS += simplifytypes
SUBDIRS += dumpers
SUBDIRS += namedemangler.pro
SUBDIRS += disassembler.pro
SUBDIRS += offsets.pro

