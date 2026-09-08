Project {
    CppApplication {
        name: "app"
        files: ["multi-target-project-main.cpp", "multi-target-project-shared.h"]
    }
    StaticLibrary {
        name: "lib"
        Depends { name: "cpp" }
        files: ["multi-target-project-lib.cpp", "multi-target-project-shared.h"]
    }
    DynamicLibrary {
        name: "dyn"
        Depends { name: "cpp" }
        Depends { name: "lib" }
        files: ["multi-target-project-dyn.cpp"]
    }
}
