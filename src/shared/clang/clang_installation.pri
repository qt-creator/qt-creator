isEmpty(LLVM_INSTALL_DIR):LLVM_INSTALL_DIR=$$(LLVM_INSTALL_DIR)
LLVM_INSTALL_DIR = $$clean_path($$LLVM_INSTALL_DIR)

!isEmpty(LLVM_INSTALL_DIR):!exists($$LLVM_INSTALL_DIR) {
    error("Explicitly given LLVM_INSTALL_DIR does not exist: $$LLVM_INSTALL_DIR")
}

defineReplace(llvmWarningOrError) {
    warningText = $$1
    errorText = $$2
    isEmpty(errorText): errorText = $$warningText
    isEmpty(LLVM_INSTALL_DIR): warning($$warningText)
    else: error($$errorText)
    return(false)
}

# Expected input: "3.9.1", "5.0.0svn", "5.0.1git-81029f14223"
defineReplace(extractVersion)      { return($$replace(1, ^(\\d+\\.\\d+\\.\\d+).*$, \\1)) }
defineReplace(extractMajorVersion) { return($$replace(1, ^(\\d+)\\.\\d+\\.\\d+.*$, \\1)) }
defineReplace(extractMinorVersion) { return($$replace(1, ^\\d+\\.(\\d+)\\.\\d+.*$, \\1)) }
defineReplace(extractPatchVersion) { return($$replace(1, ^\\d+\\.\\d+\\.(\\d+).*$, \\1)) }

defineTest(versionIsAtLeast) {
    actual_major_version = $$extractMajorVersion($$1)
    actual_minor_version = $$extractMinorVersion($$1)
    actual_patch_version = $$extractPatchVersion($$1)
    required_min_major_version = $$2
    required_min_minor_version = $$3
    required_min_patch_version = $$4

    isEqual(actual_major_version, $$required_min_major_version) {
        isEqual(actual_minor_version, $$required_min_minor_version) {
            isEqual(actual_patch_version, $$required_min_patch_version): return(true)
            greaterThan(actual_patch_version, $$required_min_patch_version): return(true)
        }
        greaterThan(actual_minor_version, $$required_min_minor_version): return(true)
    }
    greaterThan(actual_major_version, $$required_min_major_version): return(true)

    return(false)
}

defineTest(versionIsEqual) {
    actual_major_version = $$extractMajorVersion($$1)
    actual_minor_version = $$extractMinorVersion($$1)
    required_min_major_version = $$2
    required_min_minor_version = $$3

    isEqual(actual_major_version, $$required_min_major_version) {
        isEqual(actual_minor_version, $$required_min_minor_version) {
            return(true)
        }
    }

    return(false)
}

defineReplace(findLLVMVersionFromLibDir) {
    libdir = $$1
    version_dirs = $$files($$libdir/clang/*)
    for (version_dir, version_dirs) {
        fileName = $$basename(version_dir)
        version = $$find(fileName, ^(\\d+\\.\\d+\\.\\d+)$)
        !isEmpty(version): return($$version)
    }
}

defineReplace(findClangLibInLibDir) {
    libdir = $$1
    exists($${libdir}/libclang.so*)|exists($${libdir}/libclang.dylib) {
        return("-lclang")
    } else {
        exists ($${libdir}/libclang.*) {
            return("-llibclang")
        } else {
            exists ($${libdir}/liblibclang.dll.a) {
                return("-llibclang.dll")
            }
        }
    }
}

CLANGTOOLING_LIBS=-lclangTooling -lclangIndex -lclangFrontend -lclangParse -lclangSerialization \
                  -lclangSema -lclangEdit -lclangAnalysis -lclangDriver -lclangDynamicASTMatchers \
                  -lclangASTMatchers -lclangToolingCore -lclangAST -lclangLex -lclangBasic
win32:CLANGTOOLING_LIBS += -lversion

BIN_EXTENSION =
win32: BIN_EXTENSION = .exe

isEmpty(LLVM_INSTALL_DIR) {
    llvm_config = llvm-config
} else {
    llvm_config = $$system_quote($$LLVM_INSTALL_DIR/bin/llvm-config)
    requires(exists($$llvm_config$$BIN_EXTENSION))
}

output = $$system($$llvm_config --version, lines)
LLVM_VERSION = $$extractVersion($$output)
isEmpty(LLVM_VERSION) {
    $$llvmWarningOrError(\
        "Cannot determine clang version. Set LLVM_INSTALL_DIR to build the Clang Code Model",\
        "LLVM_INSTALL_DIR does not contain a valid llvm-config, candidate: $$llvm_config")
} else:!versionIsAtLeast($$LLVM_VERSION, 6, 0, 0): {
    # CLANG-UPGRADE-CHECK: Adapt minimum version numbers.
    $$llvmWarningOrError(\
        "LLVM/Clang version >= 6.0.0 required, version provided: $$LLVM_VERSION")
    LLVM_VERSION =
} else {
    # CLANG-UPGRADE-CHECK: Remove suppression if this warning is resolved.
    gcc {
        # GCC6 shows full version (6.4.0), while GCC7 and up show only major version (8)
        GCC_VERSION = $$system("$$QMAKE_CXX -dumpversion")
        GCC_MAJOR_VERSION = $$section(GCC_VERSION, ., 0, 0)
        # GCC8 warns about memset/memcpy for types with copy ctor. Clang has some of these.
        greaterThan(GCC_MAJOR_VERSION, 7):QMAKE_CXXFLAGS += -Wno-class-memaccess
    }

    LLVM_LIBDIR = $$quote($$system($$llvm_config --libdir, lines))
    LLVM_BINDIR = $$quote($$system($$llvm_config --bindir, lines))
    LLVM_INCLUDEPATH = $$system($$llvm_config --includedir, lines)
    msvc {
        # CLANG-UPGRADE-CHECK: Remove suppression if this warning is resolved.
        # Suppress unreferenced formal parameter warnings
        QMAKE_CXXFLAGS += -wd4100
        LLVM_STATIC_LIBS_STRING += $$system($$llvm_config --libnames, lines)
    } else {
        LLVM_STATIC_LIBS_STRING += $$system($$llvm_config --libs, lines)
    }
    LLVM_STATIC_LIBS_STRING += $$system($$llvm_config --system-libs, lines)

    LLVM_STATIC_LIBS = $$split(LLVM_STATIC_LIBS_STRING, " ")

    LIBCLANG_MAIN_HEADER = $$LLVM_INCLUDEPATH/clang-c/Index.h
    !exists($$LIBCLANG_MAIN_HEADER) {
        $$llvmWarningOrError(\
            "Cannot find libclang's main header file, candidate: $$LIBCLANG_MAIN_HEADER")
    }
    !exists($$LLVM_LIBDIR) {
        $$llvmWarningOrError("Cannot detect lib dir for clang, candidate: $$LLVM_LIBDIR")
    }
    !exists($$LLVM_BINDIR) {
        $$llvmWarningOrError("Cannot detect bin dir for clang, candidate: $$LLVM_BINDIR")
    }
    CLANG_LIB = $$findClangLibInLibDir($$LLVM_LIBDIR)
    isEmpty(CLANG_LIB) {
        $$llvmWarningOrError("Cannot find Clang shared library in $$LLVM_LIBDIR")
    }

    !contains(QMAKE_DEFAULT_LIBDIRS, $$LLVM_LIBDIR): LIBCLANG_LIBS = -L$${LLVM_LIBDIR}
    LIBCLANG_LIBS += $${CLANG_LIB}

    QTC_NO_CLANG_LIBTOOLING=$$(QTC_NO_CLANG_LIBTOOLING)
    isEmpty(QTC_NO_CLANG_LIBTOOLING) {
        QTC_FORCE_CLANG_LIBTOOLING = $$(QTC_FORCE_CLANG_LIBTOOLING)
        versionIsEqual($$LLVM_VERSION, 6, 0)|!isEmpty(QTC_FORCE_CLANG_LIBTOOLING) {
            !contains(QMAKE_DEFAULT_LIBDIRS, $$LLVM_LIBDIR): LIBTOOLING_LIBS = -L$${LLVM_LIBDIR}
            LIBTOOLING_LIBS += $$CLANGTOOLING_LIBS $$LLVM_STATIC_LIBS
        } else {
            warning("Clang LibTooling is disabled because only version 6.0 is supported.")
        }
    } else {
        warning("Clang LibTooling is disabled.")
    }

    contains(QMAKE_DEFAULT_INCDIRS, $$LLVM_INCLUDEPATH): LLVM_INCLUDEPATH =

    # Remove unwanted flags. It is a workaround for linking.
    # It is not intended for cross compiler linking.
    LLVM_CXXFLAGS = $$system($$llvm_config --cxxflags, lines)
    LLVM_CXXFLAGS ~= s,-fno-exceptions,
    LLVM_CXXFLAGS ~= s,-std=c++11,
    LLVM_CXXFLAGS ~= s,-std=c++0x,
    LLVM_CXXFLAGS ~= s,-O\S*,
    LLVM_CXXFLAGS ~= s,/O\S*,
    LLVM_CXXFLAGS ~= s,/W4,
    LLVM_CXXFLAGS ~= s,/EH\S*,
    LLVM_CXXFLAGS ~= s,-Werror=date-time,
    LLVM_CXXFLAGS ~= s,-Wcovered-switch-default,
    LLVM_CXXFLAGS ~= s,-fPIC,
    LLVM_CXXFLAGS ~= s,-pedantic,
    LLVM_CXXFLAGS ~= s,-Wstring-conversion,
    # split-dwarf needs objcopy which does not work via icecc out-of-the-box
    LLVM_CXXFLAGS ~= s,-gsplit-dwarf,

    LLVM_IS_COMPILED_WITH_RTTI = $$system($$llvm_config --has-rtti, lines)

    unix:!disable_external_rpath:!contains(QMAKE_DEFAULT_LIBDIRS, $${LLVM_LIBDIR}) {
        !macos: QMAKE_LFLAGS += -Wl,-z,origin
        QMAKE_LFLAGS += -Wl,-rpath,$$shell_quote($${LLVM_LIBDIR})
    }
}
