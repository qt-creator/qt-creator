import qbs

QtcAutotest {
    name: "Memcheck " + testName + " autotest"
    property string testName
    targetName: testName // Test runner hardcodes the names of the executables
    destinationDirectory: project.buildDirectory + '/'
                          + project.ide_bin_path + '/testapps/' + testName
    files: "main.cpp"
}
