import qbs.base 1.0
import "../../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "Botan"

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }

    cpp.includePaths: '.'
    cpp.dynamicLibraries: {
        if (qbs.targetOS == "windows") {
            return [
                "advapi32",
                "user32"
            ]
        } else {
            return ["rt", "dl"]
        }
    }

    cpp.defines: {
        var result = ["BOTAN_DLL=Q_DECL_EXPORT"]
        if (qbs.toolchain == "msvc")
            result.push("BOTAN_BUILD_COMPILER_IS_MSVC", "BOTAN_TARGET_OS_HAS_GMTIME_S")
        if (qbs.toolchain == "gcc" || qbs.toolchain == "mingw")
            result.push("BOTAN_BUILD_COMPILER_IS_GCC")
        if (qbs.targetOS == "linux")
            result.push("BOTAN_TARGET_OS_IS_LINUX", "BOTAN_TARGET_OS_HAS_CLOCK_GETTIME",
                "BOTAN_TARGET_OS_HAS_DLOPEN", " BOTAN_TARGET_OS_HAS_GMTIME_R",
                "BOTAN_TARGET_OS_HAS_POSIX_MLOCK", "BOTAN_HAS_DYNAMICALLY_LOADED_ENGINE",
                "BOTAN_HAS_DYNAMIC_LOADER", "BOTAN_TARGET_OS_HAS_GETTIMEOFDAY",
                "BOTAN_HAS_ALLOC_MMAP", "BOTAN_HAS_ENTROPY_SRC_DEV_RANDOM",
                "BOTAN_HAS_ENTROPY_SRC_EGD", "BOTAN_HAS_ENTROPY_SRC_FTW",
                "BOTAN_HAS_ENTROPY_SRC_UNIX", "BOTAN_HAS_MUTEX_PTHREAD", "BOTAN_HAS_PIPE_UNIXFD_IO")
        if (qbs.targetOS == "mac")
            result.push("BOTAN_TARGET_OS_IS_DARWIN", "BOTAN_TARGET_OS_HAS_GETTIMEOFDAY",
                "BOTAN_HAS_ALLOC_MMAP", "BOTAN_HAS_ENTROPY_SRC_DEV_RANDOM",
                "BOTAN_HAS_ENTROPY_SRC_EGD", "BOTAN_HAS_ENTROPY_SRC_FTW",
                "BOTAN_HAS_ENTROPY_SRC_UNIX", "BOTAN_HAS_MUTEX_PTHREAD", "BOTAN_HAS_PIPE_UNIXFD_IO")
        if (qbs.targetOS == "windows")
            result.push("BOTAN_TARGET_OS_IS_WINDOWS",
                "BOTAN_TARGET_OS_HAS_LOADLIBRARY", "BOTAN_TARGET_OS_HAS_WIN32_GET_SYSTEMTIME",
                "BOTAN_TARGET_OS_HAS_WIN32_VIRTUAL_LOCK", "BOTAN_HAS_DYNAMICALLY_LOADED_ENGINE",
                "BOTAN_HAS_DYNAMIC_LOADER", "BOTAN_HAS_ENTROPY_SRC_CAPI",
                "BOTAN_HAS_ENTROPY_SRC_WIN32", "BOTAN_HAS_MUTEX_WIN32")
        return base.concat(result)
    }

    Properties {
        condition: qbs.toolchain == "mingw"
        cpp.cxxFlags: [
            "-fpermissive",
            "-finline-functions",
            "-Wno-long-long"
        ]
    }

    files: [ "botan.h", "botan.cpp" ]

    ProductModule {
        Depends { name: "cpp" }
        cpp.linkerFlags: {
            if (qbs.toolchain == "mingw") {
                return ["-Wl,--enable-auto-import"]
            }
        }
        cpp.includePaths: '..'
    }
}
