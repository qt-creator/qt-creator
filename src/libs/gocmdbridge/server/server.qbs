Project {
    Application {
        name: "CmdBridgeServer"
        Depends { name: "go"; required: false }
        Depends { name: "qtc" }

        type: "application"
        consoleApplication: true

        condition: go.present

        go.platforms: ["linux", "windows", "darwin"]
        go.architectures: ["amd64", "arm64"]
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
}
