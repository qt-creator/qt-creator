!isEmpty(LLVM_INSTALL_DIR) {
include(../../../src/shared/clang/clang_installation.pri)
requires(!isEmpty(LIBCLANG_LIBS))

DEFINES += CLANG_UNIT_TESTS
INCLUDEPATH += $$LLVM_INCLUDEPATH
LIBS += $$LIBTOOLING_LIBS $$LIBCLANG_LIBS
}
