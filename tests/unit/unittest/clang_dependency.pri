isEmpty(LLVM_INSTALL_DIR):LLVM_INSTALL_DIR=$$(LLVM_INSTALL_DIR)
!isEmpty(LLVM_INSTALL_DIR) {
include(../../../src/shared/clang/clang_installation.pri)
requires(!isEmpty(LIBCLANG_LIBS))
equals(LLVM_IS_COMPILED_WITH_RTTI, "NO") : message("LLVM needs to be compiled with RTTI!")
requires(equals(LLVM_IS_COMPILED_WITH_RTTI, "YES"))

DEFINES += CLANG_UNIT_TESTS
INCLUDEPATH += $$LLVM_INCLUDEPATH
win32:LIBS += -lVersion
LIBS += $$LIBTOOLING_LIBS $$LIBCLANG_LIBS
}
