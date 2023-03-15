Project {
    CppApplication {
        name: "app"
        files: ["multi-target-project-main.cpp", "multi-target-project-shared.h"]
    }
    StaticLibrary {
        Depends { name: "cpp" }
        files: ["multi-target-project-lib.cpp", "multi-target-project-shared.h"]
    }
}
