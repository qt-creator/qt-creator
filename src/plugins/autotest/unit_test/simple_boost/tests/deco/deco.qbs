import qbs
import qbs.File
CppApplication {
    name: "Decorators Test"
    type: "application"
    Properties {
        condition: project.boostIncDir && File.exists(project.boostIncDir)
        cpp.includePaths: [project.boostIncDir];
    }
    files: [ "enab.h", "main.cpp" ]
}
