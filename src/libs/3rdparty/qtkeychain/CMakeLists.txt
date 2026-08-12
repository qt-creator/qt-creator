add_qtc_library(qtkeychain
  SKIP_PCH
  DEPENDS Qt::Core
  SOURCES_PREFIX qtkeychain
  SOURCES
    keychain.cpp
    keychain.h
    keychain_p.h
    qkeychain_export.h
  PUBLIC_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}
  PROPERTIES
    QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON
  SBOM_ARGS
    SBOM_ENTITY_TYPE THIRD_PARTY_LIBRARY_WITH_FILES
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      qtkeychain
)

if (WIN32)
  option(USE_CREDENTIAL_STORE "Build with windows CredentialStore support" ON)

  extend_qtc_library(qtkeychain
    SOURCES_PREFIX qtkeychain
    SOURCES keychain_win.cpp
    DEPENDS crypt32
  )

  extend_qtc_library(qtkeychain
    CONDITION USE_CREDENTIAL_STORE
    FEATURE_INFO "CredentialStore keychain support"
    DEFINES USE_CREDENTIAL_STORE=1
  )
  extend_qtc_library(qtkeychain
    CONDITION NOT USE_CREDENTIAL_STORE
    SOURCES_PREFIX qtkeychain
    SOURCES plaintextstore.cpp
  )
endif()

extend_qtc_library(qtkeychain
  CONDITION APPLE
  SOURCES_PREFIX qtkeychain
  SOURCES keychain_apple.mm
  DEPENDS ${FWFoundation} ${FWSecurity}
)

extend_qtc_library(qtkeychain
  CONDITION CMAKE_SYSTEM_NAME STREQUAL "OHOS"
  FEATURE_INFO "OHOS asset store keychain support"
  SOURCES_PREFIX qtkeychain
  SOURCES keychain_ohos.cpp
  DEPENDS asset_ndk.z
)

if (UNIX AND NOT APPLE AND NOT EMSCRIPTEN AND NOT CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    find_package(Qt6 COMPONENTS DBus)

    option(LIBSECRET_SUPPORT "Build with libsecret support if available" ON)
    if (LIBSECRET_SUPPORT)
      find_package(PkgConfig)

      include(FindPkgConfig)
      pkg_check_modules(LIBSECRET libsecret-1)

      extend_qtc_library(qtkeychain
        CONDITION LIBSECRET_FOUND
        FEATURE_INFO "libsecret keychain support"
        DEFINES HAVE_LIBSECRET=1
        INCLUDES ${LIBSECRET_INCLUDE_DIRS}
        DEPENDS ${LIBSECRET_LIBRARIES}
      )
    endif()

  # The command comes with Qt::DBus, which a Qt build can lack.
  if (TARGET Qt::DBus)
    qt6_add_dbus_interface(dbus_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/qtkeychain/org.kde.KWallet.xml
      qtkeychain/kwallet_interface KWalletInterface
    )
  endif()

  extend_qtc_library(qtkeychain
    CONDITION TARGET Qt::DBus
    FEATURE_INFO "keychain dbus support"
    DEFINES KEYCHAIN_DBUS=1
    DEPENDS Qt::DBus
    SOURCES
      qtkeychain/keychain_unix.cpp
      qtkeychain/libsecret.cpp
      qtkeychain/plaintextstore.cpp
      ${dbus_SOURCES}
    INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/qtkeychain
    )
endif()
