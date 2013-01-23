
TEMPLATE = subdirs

SUBDIRS += version.pro
SUBDIRS += dumpers.pro
#SUBDIRS += olddumpers.pro
SUBDIRS += namedemangler.pro

!win32-msvc*: SUBDIRS += gdb.pro

