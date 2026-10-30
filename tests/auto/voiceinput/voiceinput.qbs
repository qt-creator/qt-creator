import qbs

QtcAutotest {
    name: "VoiceInput autotest"

    property string voiceInputDir: project.ide_source_tree + "/src/libs/voiceinput/"

    cpp.includePaths: base.concat([voiceInputDir])

    Group {
        name: "Sources from VoiceInput"
        prefix: voiceInputDir
        files: [
            "speechactivity.cpp",
            "speechactivity.h",
        ]
    }
    Group {
        name: "Test sources"
        files: ["tst_voiceinput.cpp"]
    }
}
