isEmpty(LLVM_INSTALL_DIR):LLVM_INSTALL_DIR=$$(LLVM_INSTALL_DIR)
LLVM_INSTALL_DIR = $$clean_path($$LLVM_INSTALL_DIR)
isEmpty(LLVM_INSTALL_DIR): error("No LLVM_INSTALL_DIR provided")
!exists($$LLVM_INSTALL_DIR): error("LLVM_INSTALL_DIR does not exist: $$LLVM_INSTALL_DIR")

defineReplace(extractVersion)      { return($$replace(1, ^(\\d+\\.\\d+\\.\\d+)$, \\1)) }
defineReplace(extractMajorVersion) { return($$replace(1, ^(\\d+)\\.\\d+\\.\\d+$, \\1)) }
defineReplace(extractMinorVersion) { return($$replace(1, ^\\d+\\.(\\d+)\\.\\d+$, \\1)) }
defineReplace(extractPatchVersion) { return($$replace(1, ^\\d+\\.\\d+\\.(\\d+)$, \\1)) }

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

defineReplace(findClangOnWindows) {
    FILE_EXTS = a dll
    msvc: FILE_EXTS = lib dll
    for (suffix, $$list(lib bin)) {
        for (libname, $$list(clang libclang)) {
            for (ext, FILE_EXTS) {
                exists("$${LLVM_INSTALL_DIR}/$${suffix}/$${libname}.$${ext}") {
                    return($${LLVM_INSTALL_DIR}/$${suffix}/)
                }
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

llvm_config = $$system_quote($$LLVM_INSTALL_DIR/bin/llvm-config)
requires(exists($$llvm_config$$BIN_EXTENSION))
#message("llvm-config found, querying it for paths and version")
LLVM_LIBDIR = $$quote($$system($$llvm_config --libdir, lines))
LLVM_INCLUDEPATH = $$system($$llvm_config --includedir, lines)
output = $$system($$llvm_config --version, lines)
LLVM_VERSION = $$extractVersion($$output)
msvc {
    LLVM_STATIC_LIBS_STRING += $$system($$llvm_config --libnames, lines)
} else {
    LLVM_STATIC_LIBS_STRING += $$system($$llvm_config --libs, lines)
}
LLVM_STATIC_LIBS_STRING += $$system($$llvm_config --system-libs, lines)

LLVM_STATIC_LIBS = $$split(LLVM_STATIC_LIBS_STRING, " ")

LIBCLANG_MAIN_HEADER = $$LLVM_INCLUDEPATH/clang-c/Index.h
!exists($$LIBCLANG_MAIN_HEADER): error("Cannot find libclang's main header file, candidate: $$LIBCLANG_MAIN_HEADER")
!exists($$LLVM_LIBDIR): error("Cannot detect lib dir for clang, candidate: $$LLVM_LIBDIR")
CLANG_LIB = $$findClangLibInLibDir($$LLVM_LIBDIR)
isEmpty(CLANG_LIB): error("Cannot find Clang shared library in $$LLVM_LIBDIR")

!contains(QMAKE_DEFAULT_LIBDIRS, $$LLVM_LIBDIR): LIBCLANG_LIBS = -L$${LLVM_LIBDIR}
LIBCLANG_LIBS += $${CLANG_LIB}

QTC_NO_CLANG_LIBTOOLING=$$(QTC_NO_CLANG_LIBTOOLING)
isEmpty(QTC_NO_CLANG_LIBTOOLING) {
    !contains(QMAKE_DEFAULT_LIBDIRS, $$LLVM_LIBDIR): LIBTOOLING_LIBS = -L$${LLVM_LIBDIR}
    LIBTOOLING_LIBS += $$CLANGTOOLING_LIBS $$LLVM_STATIC_LIBS
    contains(QMAKE_DEFAULT_INCDIRS, $$LLVM_INCLUDEPATH): LLVM_INCLUDEPATH =
} else {
    warning("Clang LibTooling is disabled.")
}

isEmpty(LLVM_VERSION): error("Cannot determine clang version at $$LLVM_INSTALL_DIR")
!versionIsAtLeast($$LLVM_VERSION, 3, 9, 0): {
    error("LLVM/Clang version >= 3.9.0 required, version provided: $$LLVM_VERSION")
}

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
# split-dwarf needs objcopy which does not work via icecc out-of-the-box
LLVM_CXXFLAGS ~= s,-gsplit-dwarf,

LLVM_IS_COMPILED_WITH_RTTI = $$system($$llvm_config --has-rtti, lines)
