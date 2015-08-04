include(../../../src/shared/clang/clang_installation.pri)
requires(!isEmpty(LLVM_LIBS))

INCLUDEPATH += $$LLVM_INCLUDEPATH
LIBS += $$LLVM_LIBS
