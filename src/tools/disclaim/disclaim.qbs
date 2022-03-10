import qbs 1.0

QtcTool {
    name: "disclaim"
    condition: qbs.targetOS.contains("macos")

    files: [
        "disclaim.mm"
    ]

    installDir: qtc.ide_libexec_path
}
