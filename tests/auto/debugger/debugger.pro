
TEMPLATE = subdirs

SUBDIRS = dumpers.pro version.pro

!win32-msvc*: SUBDIRS += gdb.pro

