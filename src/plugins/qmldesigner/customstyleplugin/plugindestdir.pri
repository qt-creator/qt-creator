macx {
  DESTDIR = $$IDE_LIBRARY_PATH/QmlDesigner
} else {
  DESTDIR = $$IDE_BUILD_TREE/$${IDE_LIBRARY_BASENAME}/qmldesigner
}
