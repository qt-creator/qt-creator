// The Go implementation is multiplexed over the target platforms, as before.
// Setting project.buildCmdBridgeC builds the C implementation instead, for the
// host platform only; cross-compiling it to every platform needs zig and is
// only wired up in the CMake build
// (-DBUILD_CMDBRIDGE_C=ON -DCMDBRIDGE_USE_ZIG=ON).
Product {
    name: "CmdBridgeServer"
    type: "application"
    consoleApplication: true

    Depends { name: "go"; required: false }
    Depends { name: "qtc" }

    property bool buildCmdBridgeC: project.buildCmdBridgeC || false

    condition: buildCmdBridgeC || go.present

    // The client looks the binary up as cmdbridge-<os>-<arch>; the Go build
    // derives that from its profile, the C build targets the host.
    targetName: buildCmdBridgeC
        ? "cmdbridge-" + qbs.targetOS[0] + "-" + qbs.architecture
        : "cmdbridge"

    // The Go build needs no compiler module; the C build is compiled by cpp.
    qtc.useCpp: buildCmdBridgeC

    Profile { name: "linux-amd64"; go.platform: "linux"; go.architecture: "amd64" }
    Profile { name: "linux-arm64"; go.platform: "linux"; go.architecture: "arm64" }
    Profile { name: "windows-amd64"; go.platform: "windows"; go.architecture: "amd64" }
    Profile { name: "windows-arm64"; go.platform: "windows"; go.architecture: "arm64" }
    Profile { name: "darwin-amd64"; go.platform: "darwin"; go.architecture: "amd64" }
    Profile { name: "darwin-arm64"; go.platform: "darwin"; go.architecture: "arm64" }
    Profile { name: "freebsd-amd64"; go.platform: "freebsd"; go.architecture: "amd64" }
    Profile { name: "freebsd-arm64"; go.platform: "freebsd"; go.architecture: "arm64" }
    Profile { name: "openbsd-amd64"; go.platform: "openbsd"; go.architecture: "amd64" }
    Profile { name: "openbsd-arm64"; go.platform: "openbsd"; go.architecture: "arm64" }
    Profile { name: "netbsd-amd64"; go.platform: "netbsd"; go.architecture: "amd64" }
    Profile { name: "netbsd-arm64"; go.platform: "netbsd"; go.architecture: "arm64" }
    qbs.profiles: buildCmdBridgeC
        ? [] // host only
        : ["linux-amd64", "linux-arm64", "windows-amd64", "windows-arm64", "darwin-amd64",
           "darwin-arm64", "freebsd-amd64", "freebsd-arm64", "openbsd-amd64", "openbsd-arm64",
           "netbsd-amd64", "netbsd-arm64"]
    multiplexByQbsProperties: "profiles"

    go.magicPacketMarker: project.magicPacketMarker

    Group {
        name: "Go files"
        condition: !product.buildCmdBridgeC
        files: [
            "go/go.mod",
            "go/*.go",
            "go/go.sum",
        ]
    }

    // Only cmdbridge.c is compiled; it #includes the others (some of them
    // indirectly, e.g. watcher.c includes its platform backends), which are
    // listed separately so they show up in the project and trigger a rebuild.
    Group {
        name: "C source"
        condition: product.buildCmdBridgeC
        files: ["c/cmdbridge.c"]
    }

    Group {
        name: "C source (included by cmdbridge.c)"
        condition: product.buildCmdBridgeC
        fileTags: [] // not compiled on their own
        files: [
            "c/cbor.c",
            "c/cancel.c",
            "c/containers.c",
            "c/environment.c",
            "c/exec.c",
            "c/exec_posix.c",
            "c/exec_win.c",
            "c/fileaccess.c",
            "c/fileaccess_posix.c",
            "c/fileaccess_win.c",
            "c/fileops.c",
            "c/find.c",
            "c/find_posix.c",
            "c/find_win.c",
            "c/is.c",
            "c/readfile.c",
            "c/socketforward.c",
            "c/stat.c",
            "c/watcher.c",
            "c/watcher_apple.c",
            "c/watcher_linux.c",
            "c/watcher_none.c",
            "c/watcher_win.c",
            "c/wire.c",
            "c/writefile.c",
        ]
    }

    Properties {
        condition: product.buildCmdBridgeC
        cpp.cLanguageVersion: "c11"
        cpp.defines: [
            "_GNU_SOURCE",
            'GOBRIDGE_MAGIC_PACKET_MARKER="' + project.magicPacketMarker + '"',
        ]
        // ws2_32 for AF_UNIX sockets, bcrypt for BCryptGenRandom.
        cpp.dynamicLibraries: qbs.targetOS.contains("windows")
            ? ["ws2_32", "bcrypt"] : ["pthread"]
    }

    Group {
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: qtc.ide_libexec_path
    }
}
