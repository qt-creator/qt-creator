QtcLibrary {
    name: "qtkeychain"

    property bool useWinCredentialsStore: qbs.targetOS.contains("windows")

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.dbus"; condition: qbs.targetOS.contains("linux") }
    Depends { name: "libsecret-1"; required: false }

    cpp.defines: base.concat(["QTKEYCHAIN_LIBRARY"])

    Properties {
        condition: useWinCredentialsStore
        cpp.defines: outer.concat(["USE_CREDENTIAL_STORE=1"])
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
        name: "qtkeychain Linux files"
        condition: qbs.targetOS.contains("linux")

        Group {
            name: "qtkeychain libsecret support"
            condition: "libsecret-1".present
            cpp.defines: outer.concat(["HAVE_LIBSECRET=1"])
        }
        Group {
            name: "dbus sources"
            fileTags: "qt.dbus.interface"
            files: ["org.kde.KWallet.xml"]
        }

        Group {
            name: "qtkeychain dbus support"
            cpp.defines: outer.concat(["KEYCHAIN_DBUS=1"])
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

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: project.ide_source_tree + "/src/libs/3rdparty/"
    }
}
