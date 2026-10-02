# Minimal qmake support.
# This file is provided as is without any warranty.
# It can break at anytime or be removed without notice.

lessThan(QT_MAJOR_VERSION, 5) {
    error("qtkeychain requires Qt 5 or later")
}

QTKEYCHAIN_PWD = $$PWD/qtkeychain

CONFIG += depend_includepath
DEFINES += QTKEYCHAIN_NO_EXPORT

INCLUDEPATH += \
    $$PWD/.. \
    $$QTKEYCHAIN_PWD

HEADERS += \
    $$QTKEYCHAIN_PWD/keychain_p.h \
    $$QTKEYCHAIN_PWD/keychain.h

SOURCES += \
    $$QTKEYCHAIN_PWD/keychain.cpp

unix:!android:!macx:!ios {
    # Remove the following LIBSECRET_SUPPORT line
    # to build without libsecret support.
    DEFINES += LIBSECRET_SUPPORT
    contains(DEFINES, LIBSECRET_SUPPORT) {
        packagesExist(libsecret-1) {
            !build_pass:message("Libsecret support: on")
            CONFIG += link_pkgconfig
            PKGCONFIG += libsecret-1
            DEFINES += HAVE_LIBSECRET
        } else {
            !build_pass:warning("Libsecret not found.")
            !build_pass:message("Libsecret support: off")
        }
    } else {
        !build_pass:message("Libsecret support: off")
    }

    # Generate D-Bus interface:
    DEFINES += KEYCHAIN_DBUS
    QT += dbus
    kwallet_interface.files = $$QTKEYCHAIN_PWD/org.kde.KWallet.xml
    DBUS_INTERFACES += kwallet_interface

    HEADERS += \
        $$QTKEYCHAIN_PWD/gnomekeyring_p.h \
        $$QTKEYCHAIN_PWD/plaintextstore_p.h \
        $$QTKEYCHAIN_PWD/libsecret_p.h
    SOURCES += \
        $$QTKEYCHAIN_PWD/keychain_unix.cpp \
        $$QTKEYCHAIN_PWD/plaintextstore.cpp \
        $$QTKEYCHAIN_PWD/gnomekeyring.cpp \
        $$QTKEYCHAIN_PWD/libsecret.cpp
}

android {
    lessThan(QT_MAJOR_VERSION, 6) {
        QT += androidextras
    }

    HEADERS += \
        $$QTKEYCHAIN_PWD/androidkeystore_p.h \
        $$QTKEYCHAIN_PWD/plaintextstore_p.h
    SOURCES += \
        $$QTKEYCHAIN_PWD/androidkeystore.cpp \
        $$QTKEYCHAIN_PWD/keychain_android.cpp \
        $$QTKEYCHAIN_PWD/plaintextstore.cpp
}

win32 {
    # Remove the following USE_CREDENTIAL_STORE line
    # to use the CryptProtectData Windows API function
    # instead of the Windows Credential Store.
    DEFINES += USE_CREDENTIAL_STORE
    contains(DEFINES, USE_CREDENTIAL_STORE) {
        !build_pass:message("Windows Credential Store support: on")
        LIBS += -ladvapi32 -lcrypt32
    } else {
        !build_pass:message("Windows Credential Store support: off")
        LIBS += -lcrypt32
        HEADERS += $$QTKEYCHAIN_PWD/plaintextstore_p.h
        SOURCES += $$QTKEYCHAIN_PWD/plaintextstore.cpp
    }
    HEADERS += $$QTKEYCHAIN_PWD/libsecret_p.h
    SOURCES += \
        $$QTKEYCHAIN_PWD/keychain_win.cpp \
        $$QTKEYCHAIN_PWD/libsecret.cpp
}

macx|ios {
    LIBS += -framework Security -framework Foundation
    OBJECTIVE_SOURCES += $$QTKEYCHAIN_PWD/keychain_apple.mm
}
