add_qtc_library(quickjsng
  PROPERTIES QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON C_STANDARD 11 C_STANDARD_REQUIRED ON
  PUBLIC_INCLUDES src
  STATIC
  SOURCES
    src/builtin-array-fromasync.h
    src/builtin-iterator-zip.h
    src/builtin-iterator-zip-keyed.h
    src/cutils.h
    src/dtoa.c
    src/dtoa.h
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
  SBOM_ARGS
    SBOM_ENTITY_TYPE THIRD_PARTY_LIBRARY_WITH_FILES
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      quickjsng
)

extend_qtc_library(quickjsng CONDITION WIN32 DEFINES WIN32_LEAN_AND_MEAN)
