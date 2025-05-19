Application {
    name: "CmdBridgeServer"
    consoleApplication: true
    condition: go.present

    Depends { name: "go"; required: false }
    Depends { name: "qtc" }

    Profile { name: "linux-amd64"; go.platform: "linux"; go.architecture: "amd64" }
    Profile { name: "linux-arm64"; go.platform: "linux"; go.architecture: "arm64" }
    Profile { name: "windows-amd64"; go.platform: "windows"; go.architecture: "amd64" }
    Profile { name: "windows-arm64"; go.platform: "windows"; go.architecture: "arm64" }
    Profile { name: "darwin-amd64"; go.platform: "darwin"; go.architecture: "amd64" }
    Profile { name: "darwin-arm64"; go.platform: "darwin"; go.architecture: "arm64" }
    qbs.profiles: ["linux-amd64", "linux-arm64", "windows-amd64", "windows-arm64", "darwin-amd64",
        "darwin-arm64"]
    multiplexByQbsProperties: "profiles"

    go.magicPacketMarker: project.magicPacketMarker

    Group {
        name: "Go files"
        files: [
            "go.mod",
            "*.go",
            "go.sum",
        ]
    }

    targetName: "cmdbridge"
    installDir:  qtc.ide_libexec_path
    install: true
}
