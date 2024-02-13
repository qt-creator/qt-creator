import qbs.FileInfo

Project  {
    QtcAutotest {
        name: "QProcessTask autotest"

        Depends { name: "Qt"; submodules: ["network"] }
        Depends { name: "Tasking" }

        files: [
            "tst_qprocesstask.cpp",
        ]
        cpp.defines: {
            var defines = base;
            if (qbs.targetOS === "windows")
                defines.push("_CRT_SECURE_NO_WARNINGS");
            defines.push('PROCESS_TESTAPP="'
                         + FileInfo.joinPaths(destinationDirectory, "testsleep/testsleep") + '"');
            return defines;
        }
        destinationDirectory: project.buildDirectory + '/'
                              + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)
    }
    references: "testsleep/testsleep.qbs"
}
