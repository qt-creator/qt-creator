macx {
  DESTDIR = $$IDE_PLUGIN_PATH/QmlDesigner
} else {
  DESTDIR = $$IDE_PLUGIN_PATH/qmldesigner
  target.path  = $$INSTALL_PLUGIN_PATH/qmldesigner
  INSTALLS    += target
}
