import qbs
import qbs.File
CppApplication {
    name: "Fixture Test"
    type: "application"
    Properties {
        condition: project.boostIncDir && File.exists(project.boostIncDir)
        cpp.includePaths: [project.boostIncDir];
    }
    files: [ "fix.cpp" ]
}
