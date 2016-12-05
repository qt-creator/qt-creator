import qbs

QtcAutotest {
    type: ["application"] // Not to be executed directly by autotest-runner
    name: "Memcheck " + testName + " autotest"
    cpp.warningLevel: "none"
    property string testName
    targetName: testName // Test runner hardcodes the names of the executables
    destinationDirectory: project.buildDirectory + '/'
                          + qtc.ide_bin_path + '/testapps/' + testName
    files: sourceDirectory + "/main.cpp"
}
