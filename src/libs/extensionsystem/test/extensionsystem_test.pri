RELATIVEPATH = $$RELATIVEPATH/../../../..
INCLUDEPATH *= $$PWD/../..
macx {
    LIBS += -L"$$OUT_PWD/$$RELATIVEPATH/bin/Qt Creator.app/Contents/PlugIns"
}
else {
    LIBS += $$OUT_PWD/$$RELATIVEPATH/lib
}

# stolen from qtcreator.pri
defineReplace(qtLibraryName) {
   unset(LIBRARY_NAME)
   LIBRARY_NAME = $$1
   CONFIG(debug, debug|release) {
      !debug_and_release|build_pass {
          mac:RET = $$member(LIBRARY_NAME, 0)_debug
              else:win32:RET = $$member(LIBRARY_NAME, 0)d
      }
   }
   isEmpty(RET):RET = $$LIBRARY_NAME
   return($$RET)
}

include(../extensionsystem.pri)

QT *= xml

