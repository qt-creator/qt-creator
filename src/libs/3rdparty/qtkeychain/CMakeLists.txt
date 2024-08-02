add_qtc_library(qtkeychain
  DEPENDS Qt::Core
  SOURCES
    keychain.cpp keychain.h
    qkeychain_export.h
  PROPERTIES
    QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON
)

if (WIN32)
  option(USE_CREDENTIAL_STORE "Build with windows CredentialStore support" ON)

  extend_qtc_library(qtkeychain SOURCES keychain_win.cpp)

  extend_qtc_library(qtkeychain
    CONDITION USE_CREDENTIAL_STORE
    FEATURE_INFO "CredentialStore keychain support"
    DEFINES USE_CREDENTIAL_STORE=1
  )
  extend_qtc_library(qtkeychain
    CONDITION NOT USE_CREDENTIAL_STORE
    SOURCES plaintextstore.cpp
    DEPENDS crypt32
  )
endif()

extend_qtc_library(qtkeychain
  CONDITION APPLE
  SOURCES keychain_apple.mm
  DEPENDS ${FWFoundation} ${FWSecurity}
)

if (UNIX AND NOT APPLE)
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

  qt6_add_dbus_interface(dbus_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.KWallet.xml kwallet_interface KWalletInterface)

  extend_qtc_library(qtkeychain
    CONDITION TARGET Qt::DBus
    FEATURE_INFO "keychain dbus support"
    DEFINES KEYCHAIN_DBUS=1
    DEPENDS Qt::DBus
    SOURCES
      keychain_unix.cpp
      libsecret.cpp
      plaintextstore.cpp
      ${dbus_SOURCES}
    )
endif()
