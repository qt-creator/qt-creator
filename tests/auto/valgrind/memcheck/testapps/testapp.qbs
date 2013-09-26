import qbs
import "../../../autotest.qbs" as Autotest

Autotest {
    name: "Memcheck " + testName + " autotest"
    condition: !qbs.targetOS.contains("windows")
    targetName: testName // Test runner hardcodes the names of the executables
    destinationDirectory: buildDirectory + '/' + project.ide_bin_path + '/testapps/' + testName
    files: "main.cpp"
}
