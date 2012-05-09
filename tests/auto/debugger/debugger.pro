
TEMPLATE = subdirs

SUBDIRS = dumpers.pro version.pro namedemangler.pro

!win32-msvc*: SUBDIRS += gdb.pro

