import qbs 1.0

Project {
    name: "QtcSsh"

    QtcLibrary {
        cpp.defines: {
            var defines = base;
            defines.push("QTCSSH_LIBRARY");
            if (project.withAutotests && !defines.contains("WITH_TESTS"))
                defines.push("WITH_TESTS");
            return defines;
        }
        cpp.enableExceptions: true

        Depends { name: "Qt"; submodules: ["widgets", "network" ] }
        Depends { name: "Utils" }

        files: [
            "ssh.qrc",
            "sshparameters.cpp",
            "sshparameters.h",
            "sshsettings.cpp",
            "sshsettings.h",
        ]

        Export { Depends { name: "Qt.network" } }
    }
}
