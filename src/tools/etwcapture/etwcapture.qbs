import qbs.FileInfo

Project {
    name: "EtwCapture"
    condition: qbs.targetOS.contains("windows")

    // Records the NT Kernel Logger, which needs administrator rights.
    QtcTool {
        name: "etwcapture"

        Depends { name: "Profiler" }
        Depends { name: "Utils" }
        Depends { name: "Tracing"; required: false }

        condition: Tracing.present

        files: "etwcapture.cpp"
    }

    // Pure Win32/C++ with no Qt dependencies. Its resource script embeds the
    // requireAdministrator manifest, which is what makes ShellExecuteEx elevate it.
    QtcTool {
        name: "etwcapture-launcher"
        useQt: false
        consoleApplication: false

        // etwcapture.exe is not next to the Profiler plugin it links, so the
        // launcher puts the plugin directory on the PATH it hands to it.
        cpp.defines: base.concat(['ETWCAPTURE_PLUGIN_PATH=L"'
                                  + FileInfo.relativePath('/' + qtc.ide_libexec_path,
                                                          '/' + qtc.ide_plugin_path) + '"'])

        Properties {
            condition: qbs.toolchain.contains("msvc")
            cpp.generateManifestFile: false
        }

        files: [
            "etwcapture-launcher.cpp",
            "etwcapture-launcher.rc",
        ]
    }
}
