include(../../../src/shared/clang/clang_installation.pri)
include(../../../src/shared/clang/clang_defines.pri)

!isEmpty(LLVM_VERSION) {
    requires(!isEmpty(LIBCLANG_LIBS))
    equals(LLVM_IS_COMPILED_WITH_RTTI, "NO") : message("LLVM needs to be compiled with RTTI!")
    requires(equals(LLVM_IS_COMPILED_WITH_RTTI, "YES"))

    DEFINES += CLANG_UNIT_TESTS
    INCLUDEPATH += $$LLVM_INCLUDEPATH
    win32 {
        # set run path for clang.dll dependency
        bin_path = $$LLVM_BINDIR
        bin_path ~= s,/,\\,g
        # the below gets added to later by testcase.prf
        check.commands = cd . & set PATH=$$bin_path;%PATH%& cmd /c
    }

    LIBS += $$ALL_CLANG_LIBS

    !contains(QMAKE_DEFAULT_LIBDIRS, $$LLVM_LIBDIR): QMAKE_RPATHDIR += $$LLVM_LIBDIR

    LLVM_CXXFLAGS ~= s,-g\d?,
    QMAKE_CXXFLAGS_WARN_ON *= $$LLVM_CXXFLAGS_WARNINGS
    QMAKE_CXXFLAGS *= $$LLVM_CXXFLAGS

    DEFINES += CLANG_COMPILER_PATH=\"R\\\"xxx($$LLVM_BINDIR/clang)xxx\\\"\"
}
