macx {
  DESTDIR = $$IDE_PLUGIN_PATH/QmlDesigner
} else {
  DESTDIR = $$IDE_PLUGIN_PATH/qmldesigner
  target.path  = $$QTC_PREFIX/$$IDE_LIBRARY_BASENAME/qtcreator/plugins/qmldesigner
  INSTALLS    += target
}
