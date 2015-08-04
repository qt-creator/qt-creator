isEmpty(LLVM_INSTALL_DIR):LLVM_INSTALL_DIR=$$(LLVM_INSTALL_DIR)
LLVM_INSTALL_DIR = $$clean_path($$LLVM_INSTALL_DIR)
isEmpty(LLVM_INSTALL_DIR): error("No LLVM_INSTALL_DIR provided")
!exists($$LLVM_INSTALL_DIR): error("LLVM_INSTALL_DIR does not exist: $$LLVM_INSTALL_DIR")

defineReplace(extractVersion)      { return($$replace(1, ^(\\d+\\.\\d+\\.\\d+)$, \\1)) }
defineReplace(extractMajorVersion) { return($$replace(1, ^(\\d+)\\.\\d+\\.\\d+$, \\1)) }
defineReplace(extractMinorVersion) { return($$replace(1, ^\\d+\\.(\\d+)\\.\\d+$, \\1)) }

defineTest(versionIsAtLeast) {
    actual_major_version = $$extractMajorVersion($$1)
    actual_minor_version = $$extractMinorVersion($$1)
    required_min_major_version = $$2
    required_min_minor_version = $$3

    isEqual(actual_major_version, $$required_min_major_version) {
        isEqual(actual_minor_version, $$required_min_minor_version): return(true)
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
    exists ($${libdir}/libclang.*) {
        #message("LLVM was build with autotools")
        return("clang")
    } else {
        exists ($${libdir}/liblibclang.*) {
            #message("LLVM was build with CMake")
            return("libclang")
        }
    }
}

defineReplace(findClangOnWindows) {
    FILE_EXTS = a dll
    win32-msvc*: FILE_EXTS = lib dll
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

win32 {
    LLVM_INCLUDEPATH = "$$LLVM_INSTALL_DIR/include"
    LLVM_LIBDIR = $$findClangOnWindows()
    isEmpty(LLVM_LIBDIR): error("Cannot find clang shared library at $${LLVM_INSTALL_DIR}")
    LLVM_VERSION = $$findLLVMVersionFromLibDir($$LLVM_LIBDIR)

    clang_lib = clang
    !exists("$${LLVM_LIBDIR}/clang.*"): clang_lib = libclang

    LLVM_LIBS = -L"$${LLVM_LIBDIR}" -l$${clang_lib}
    LLVM_LIBS += -ladvapi32 -lshell32
}

unix {
    llvm_config = $$LLVM_INSTALL_DIR/bin/llvm-config
    exists($$llvm_config) {
        #message("llvm-config found, querying it for paths and version")
        LLVM_LIBDIR = $$system($$llvm_config --libdir 2>/dev/null)
        LLVM_INCLUDEPATH = $$system($$llvm_config --includedir 2>/dev/null)
        output = $$system($$llvm_config --version 2>/dev/null)
        LLVM_VERSION = $$extractVersion($$output)
    } else {
        #message("llvm-config not found, concluding paths and version from LLVM_INSTALL_DIR")
        LLVM_INCLUDEPATH = $$LLVM_INSTALL_DIR/include
        LLVM_LIBDIR = $$LLVM_INSTALL_DIR/lib
        LLVM_VERSION = $$findLLVMVersionFromLibDir($$LLVM_LIBDIR)
    }

    !exists($$LLVM_INCLUDEPATH): error("Cannot detect include dir for clang, candidate: $$LLVM_INCLUDEPATH")
    !exists($$LLVM_LIBDIR): error("Cannot detect lib dir for clang, candidate: $$LLVM_LIBDIR")
    clang_lib = $$findClangLibInLibDir($$LLVM_LIBDIR)
    isEmpty(clang_lib): error("Cannot find Clang shared library in $$LLVM_LIBDIR")

    LLVM_LIBS = -L$${LLVM_LIBDIR} -l$${clang_lib}
}

isEmpty(LLVM_VERSION): error("Cannot determine clang version at $$LLVM_INSTALL_DIR")
!versionIsAtLeast($$LLVM_VERSION, 3, 6): {
    error("LLVM/Clang version >= 3.6.0 required, version provided: $$LLVM_VERSION")
}
