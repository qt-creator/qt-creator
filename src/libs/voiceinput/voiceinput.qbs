import qbs
import qbs.Environment
import qbs.File
import qbs.FileInfo

QtcLibrary {
    name: "VoiceInput"

    // Transcribes on this computer with whisper.cpp, which qbs does not build:
    // WHISPER_INSTALL_DIR points at a static installation of it.
    Probe {
        id: whisperProbe
        property string installBase: Environment.getEnv("WHISPER_INSTALL_DIR")
        property bool windows: qbs.targetOS.contains("windows")
        property bool found
        property stringList libraries
        configure: {
            libraries = [];
            found = !!installBase
                    && File.exists(FileInfo.joinPaths(installBase, "include", "whisper.h"));
            if (!found)
                return;
            // The backends depend on how whisper.cpp was configured.
            var names = ["whisper", "ggml", "ggml-cpu", "ggml-metal", "ggml-blas", "ggml-base"];
            for (var i = 0; i < names.length; ++i) {
                var file = windows ? names[i] + ".lib" : "lib" + names[i] + ".a";
                if (File.exists(FileInfo.joinPaths(installBase, "lib", file)))
                    libraries.push(names[i]);
            }
        }
    }

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Qt.multimedia"; required: false }
    Depends { name: "Utils" }

    condition: whisperProbe.found && Qt.multimedia.present

    cpp.defines: {
        var defines = base.concat("VOICEINPUT_LIBRARY");
        return defines;
    }
    cpp.includePaths: base.concat([FileInfo.joinPaths(whisperProbe.installBase, "include")])
    cpp.libraryPaths: base.concat([FileInfo.joinPaths(whisperProbe.installBase, "lib")])
    cpp.staticLibraries: base.concat(whisperProbe.libraries)
    cpp.frameworks: qbs.targetOS.contains("macos")
        ? base.concat(["Accelerate", "Foundation", "Metal", "MetalKit"])
        : base

    Qt.core.resourceSourceBase: sourceDirectory
    Qt.core.resourcePrefix: "/voiceinput"

    files: [
        "speechactivity.cpp",
        "speechactivity.h",
        "speechmodel.cpp",
        "speechmodel.h",
        "transcriber.cpp",
        "transcriber.h",
        "voiceinput_global.h",
        "voiceinputbutton.cpp",
        "voiceinputbutton.h",
        "voiceinputtr.h",
        "voicelevelmeter.cpp",
        "voicelevelmeter.h",
        "voicesettings.cpp",
        "voicesettings.h",
    ]

    Group {
        name: "images"
        prefix: "images/"
        files: [
            "microphone.png",
            "microphone@2x.png",
        ]
        fileTags: "qt.core.resource_data"
    }
}
