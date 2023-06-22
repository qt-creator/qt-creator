import qbs 1.0

QtcTool {
    name: "disclaim"
    condition: qbs.targetOS.contains("macos")
    useQt: false

    files: [
        "disclaim.mm"
    ]

    installDir: qtc.ide_libexec_path
}
