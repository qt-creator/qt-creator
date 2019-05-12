install(
  DIRECTORY cplusplus debugger glsl modeleditor qml qmldesigner
    qmlicons qml-type-descriptions schemes snippets styles templates themes welcomescreen
  DESTINATION "${IDE_DATA_PATH}"
)

install(
  FILES indexer_preincludes/qglobal.h indexer_preincludes/windows.h
  DESTINATION "${IDE_DATA_PATH}/indexer_preincludes"
)
