
TEMPLATE = subdirs

SUBDIRS = dumpers.pro plugin.pro version.pro

!win32-msvc*: SUBDIRS += gdb.pro

