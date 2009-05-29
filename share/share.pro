TEMPLATE = subdirs
SUBDIRS = qtcreator/static.pro qtcreator/translations

win32 {
  SUBDIRS += qtcreator/qtcdebugger
}
