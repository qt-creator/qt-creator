import qbs

QtcTestApp {
    name: "Memcheck " + testName + " autotest"

    Depends { name: "qtc" }

    property string testName
    cpp.warningLevel: "none"
    targetName: testName // Test runner hardcodes the names of the executables
    destinationDirectory: project.buildDirectory + '/'
                          + qtc.ide_bin_path + '/testapps/' + testName
    files: sourceDirectory + "/main.cpp"
}
