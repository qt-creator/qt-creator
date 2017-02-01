isEmpty(LLVM_INSTALL_DIR):LLVM_INSTALL_DIR=$$(LLVM_INSTALL_DIR)
!isEmpty(LLVM_INSTALL_DIR) {
include(../../../src/shared/clang/clang_installation.pri)
requires(!isEmpty(LIBCLANG_LIBS))
equals(LLVM_IS_COMPILED_WITH_RTTI, "NO") : message("LLVM needs to be compiled with RTTI!")
requires(equals(LLVM_IS_COMPILED_WITH_RTTI, "YES"))

DEFINES += CLANG_UNIT_TESTS
INCLUDEPATH += $$LLVM_INCLUDEPATH
win32 {
    LIBS += -lVersion

    # set run path for clang.dll dependency
    bin_path = $$LLVM_INSTALL_DIR/bin
    bin_path ~= s,/,\\,g
    # the below gets added to later by testcase.prf
    check.commands = cd . & set PATH=$$bin_path;%PATH%& cmd /c
}

LIBS += $$LIBTOOLING_LIBS $$LIBCLANG_LIBS
QMAKE_RPATHDIR += $$LLVM_LIBDIR

LLVM_CXXFLAGS ~= s,-g,
QMAKE_CXXFLAGS += $$LLVM_CXXFLAGS
}

DEFINES += CLANG_COMPILER_PATH=\"R\\\"xxx($$LLVM_INSTALL_DIR/bin/clang)xxx\\\"\"
