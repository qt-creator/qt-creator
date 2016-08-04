include(../../../src/shared/clang/clang_installation.pri)
requires(!isEmpty(LIBCLANG_LIBS))

INCLUDEPATH += $$LLVM_INCLUDEPATH
LIBS += $$LIBTOOLING_LIBS $$LIBCLANG_LIBS
