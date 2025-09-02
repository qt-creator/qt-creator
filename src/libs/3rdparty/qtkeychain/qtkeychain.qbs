import qbs.Utilities

QtcLibrary {
    name: "qtkeychain"

    property bool useWinCredentialsStore: qbs.targetOS.contains("windows")
    property bool useDBus: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.dbus"; condition: useDBus }
    Depends { id: libsecret; name: "libsecret-1"; required: false }

    Properties { cpp.defines: base.concat(["QTKEYCHAIN_LIBRARY"]) }

    Properties {
        condition: useWinCredentialsStore
        cpp.defines: "USE_CREDENTIAL_STORE=1"
        cpp.dynamicLibraries: ["advapi32"]
    }

    Properties {
        condition: qbs.targetOS.contains("windows") && !useWinCredentialsStore
        cpp.dynamicLibraries: ["crypt32"]
    }

    Properties {
        condition: qbs.targetOS.contains("macos")
        cpp.frameworks: [ "Foundation", "Security" ]
    }

    Properties {
        condition: libsecret.present
        cpp.defines: "HAVE_LIBSECRET=1"
    }

    Properties {
        condition: useDBus
        cpp.defines: "KEYCHAIN_DBUS=1"
    }

    Properties {
        condition: Utilities.versionCompare(qbs.version, "2.6") >= 0
        qbsModuleProviders: ["Qt", "qbspkgconfig"]
    }
    Properties { qbsModuleProviders: undefined }

    files: [
        "keychain.cpp",
        "keychain.h",
        "keychain_p.h",
        "qkeychain_export.h",
    ]

    Group {
        name: "qtkeychain Windows files"
        condition: qbs.targetOS.contains("windows")
        files: [
            "keychain_win.cpp",
            "plaintextstore_p.h",
        ]

        Group {
            name: "qtkeychain Windows no credentials store"
            condition: !product.useWinCredentialsStore
            files: [ "plaintextstore.cpp" ]
        }
    }

    Group {
        name: "qtkeychain macOS files"
        condition: qbs.targetOS.contains("macos")
        files: [ "keychain_apple.mm" ]
    }

    Group {
        name: "qtkeychain DBUS files"
        condition: useDBus

        Group {
            name: "dbus sources"
            fileTags: "qt.dbus.interface"
            files: ["org.kde.KWallet.xml"]
        }

        Group {
            name: "qtkeychain dbus support"
            cpp.cxxFlags: outer.concat("-Wno-cast-function-type")
            files: [
                "keychain_unix.cpp",
                "libsecret.cpp",
                "libsecret_p.h",
                "plaintextstore.cpp",
                "plaintextstore_p.h",
            ]
        }
    }

    Group {
        name: "CMake helpers"
        files: "cmake/**/*"
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: project.ide_source_tree + "/src/libs/3rdparty/"
    }
}
