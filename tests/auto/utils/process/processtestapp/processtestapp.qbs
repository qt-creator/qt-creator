import qbs.FileInfo

QtApplication {
    name: "processtestapp"
    Depends { name: "app_version_header" }
    Depends { name: "qtc" }
    Depends { name: "Utils" }

    consoleApplication: true
    cpp.cxxLanguageVersion: "c++17"
    cpp.defines: {
        var defines = base;
        var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                qtc.ide_libexec_path);
        var relLibExecPath = FileInfo.relativePath(destinationDirectory, absLibExecPath);
        defines.push('TEST_RELATIVE_LIBEXEC_PATH="' + relLibExecPath + '"');
        defines.push('PROCESS_TESTAPP="' + destinationDirectory + '"');
        return defines;
    }
    cpp.rpaths: project.buildDirectory + '/' + qtc.ide_library_path

    install: false
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)

    files: [
        "main.cpp",
        "processtestapp.cpp",
        "processtestapp.h",
    ]
}
