import qbs

// The symbolicator drives mach + CoreSymbolication, so it only exists on macOS.
Project {
    name: "QtProfiler symbolicator autotest"
    condition: qbs.targetOS.contains("macos")

    // A loadable module the test dlopen()s at runtime to exercise symbolication
    // of a library loaded after a session starts. Deliberately not linked into
    // the test, so it is absent until the test loads it.
    property string fixtureDir: buildDirectory + "/symbolicatorfixture"

    DynamicLibrary {
        name: "tst_symbolicator_fixture"
        Depends { name: "cpp" }
        cpp.cxxLanguageVersion: "c++20"
        targetName: "symbolicatorfixture"
        destinationDirectory: project.fixtureDir
        install: false
        files: ["fixture.cpp"]
    }

    QtcAutotest {
        name: "QtProfiler symbolicator autotest"
        Depends { name: "Utils" }
        cpp.includePaths: base.concat([path + "/../../../../src/plugins/profiler"])
        cpp.defines: base.concat([
            'SYMBOLICATOR_FIXTURE_DYLIB="' + project.fixtureDir + '/libsymbolicatorfixture.dylib"'
        ])
        files: [
            "tst_symbolicator.cpp",
            "../../../../src/plugins/profiler/symbolicator.cpp",
            "../../../../src/plugins/profiler/symbolicator.h",
        ]
    }
}
