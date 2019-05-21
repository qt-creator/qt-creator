import qbs
import qbs.File
CppApplication {
    name: "Using Test Functions"
    type: "application"
    Properties {
        condition: project.boostIncDir && File.exists(project.boostIncDir)
        cpp.includePaths: [project.boostIncDir];
    }
    files: [ "main.cpp" ]
}
