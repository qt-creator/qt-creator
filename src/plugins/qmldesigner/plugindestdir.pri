macx {
  DESTDIR = $$IDE_LIBRARY_PATH/QmlDesigner
} else {
  DESTDIR = $$IDE_LIBRARY_PATH/qmldesigner
  target.path  = $$QTC_PREFIX/$$IDE_LIBRARY_BASENAME/qtcreator/qmldesigner
  INSTALLS    += target
}
