cmake_minimum_required(VERSION 3.16)

set(KF_VERSION "6.10.0") # handled by release scripts
project(KArchive VERSION ${KF_VERSION})

include(FeatureSummary)
find_package(ECM 6.9.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

option(WITH_BZIP2 "Make bzip2 required" ON)
option(WITH_LIBLZMA "Make liblzma required" ON)
option(WITH_LIBZSTD "Make libzstd required" ON)

set(PKGCONFIG_REQUIRED_TYPE "")

if(WITH_BZIP2)
    set(BZIP2_PACKAGE_TYPE "REQUIRED")
else()
    set(BZIP2_PACKAGE_TYPE "RECOMMENDED")
endif()

if(WITH_LIBLZMA)
    set(LIBLZMA_PACKAGE_TYPE "REQUIRED")
else()
    set(LIBLZMA_PACKAGE_TYPE "RECOMMENDED")
endif()

if(WITH_LIBZSTD)
    set(PKGCONFIG_REQUIRED_TYPE "REQUIRED")
    set(LIBZSTD_REQUIRED_TYPE "REQUIRED")
else()
    set(LIBZSTD_REQUIRED_TYPE "")
endif()

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(KDEGitCommitHooks)

include(ECMGenerateExportHeader)

set(REQUIRED_QT_VERSION 6.6.0)
find_package(Qt6Core ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE)

find_package(ZLIB)
set_package_properties(ZLIB PROPERTIES
    URL "https://www.zlib.net"
    DESCRIPTION "Support for gzip compressed files and data streams"
    TYPE REQUIRED
    PURPOSE "Support for gzip compressed files and data streams"
)

find_package(BZip2)
set_package_properties(BZip2 PROPERTIES
    URL "https://sourceware.org/bzip2/"
    DESCRIPTION "Support for BZip2 compressed files and data streams"
    TYPE ${BZIP2_PACKAGE_TYPE}
    PURPOSE "Support for BZip2 compressed files and data streams"
)

find_package(LibLZMA)
set_package_properties(LibLZMA PROPERTIES
    URL "https://tukaani.org/xz/"
    DESCRIPTION "Support for xz compressed files and data streams"
    TYPE ${LIBLZMA_PACKAGE_TYPE}
    PURPOSE "Support for xz compressed files and data streams"
)


find_package(PkgConfig ${PKGCONFIG_REQUIRED_TYPE})
if (PkgConfig_FOUND)
    pkg_check_modules(LibZstd ${LIBZSTD_REQUIRED_TYPE} IMPORTED_TARGET "libzstd")
endif()
add_feature_info(LibZstd LibZstd_FOUND
                "Support for zstd compressed files and data streams"
)

include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMQtDeclareLoggingCategory)
include(ECMAddQch)
include(ECMDeprecationSettings)
include(ECMPoQmTools)

set(EXCLUDE_DEPRECATED_BEFORE_AND_AT 0 CACHE STRING "Control the range of deprecated API excluded from the build [default=0].")

option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

set(karchive_version_header "${CMAKE_CURRENT_BINARY_DIR}/src/karchive_version.h")
ecm_setup_version(PROJECT
    VARIABLE_PREFIX KARCHIVE
    VERSION_HEADER "${karchive_version_header}"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF6ArchiveConfigVersion.cmake"
    SOVERSION 6)

ecm_set_disabled_deprecation_versions(
    QT 6.8
)


add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(tests)
endif()

ecm_install_po_files_as_qm(poqm)

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF6Archive")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KF6Archive_QCH
        FILE KF6ArchiveQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KF6ArchiveQchTargets.cmake\")")
endif()

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KF6ArchiveConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KF6ArchiveConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES ${karchive_version_header}
        DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/KArchive
        COMPONENT Devel)

install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/KF6ArchiveConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/KF6ArchiveConfigVersion.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel)

install(EXPORT KF6ArchiveTargets
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        FILE KF6ArchiveTargets.cmake
        NAMESPACE KF6::)

include(ECMFeatureSummary)
ecm_feature_summary(WHAT ALL   FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
