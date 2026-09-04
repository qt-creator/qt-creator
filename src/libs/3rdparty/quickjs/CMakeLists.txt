add_qtc_library(quickjsng
  PROPERTIES QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON
  PUBLIC_INCLUDES src
  STATIC
  SOURCES
    src/builtin-array-fromasync.h
    src/cutils.c
    src/cutils.h
    src/libregexp-opcode.h
    src/libregexp.c
    src/libregexp.h
    src/libunicode-table.h
    src/libunicode.c
    src/libunicode.h
    src/list.h
    src/quickjs-atom.h
    src/quickjs-c-atomics.h
    src/quickjs-opcode.h
    src/quickjs.c
    src/quickjs.h
    src/xsum.c
    src/xsum.h
  SBOM_ARGS
    SBOM_ENTITY_TYPE THIRD_PARTY_LIBRARY_WITH_FILES
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      quickjsng
)
