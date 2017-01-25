import qbs
@if "%{TestFrameWork}" == "GTest"
import qbs.Environment
@endif

Project {
    name: "auto tests"

@if "%{TestFrameWork}" == "GTest"
    property string googletestDir: {
        if (typeof Environment.getEnv("GOOGLETEST_DIR") === 'undefined') {
            console.warn("Using googletest src dir specified at Qt Creator wizard")
            console.log("set GOOGLETEST_DIR as environment variable or Qbs property to get rid of this message")
            return "%{GTestRepository}"
        } else {
            return Environment.getEnv("GOOGLETEST_DIR")
        }
    }
@endif
@if "%{BuildAutoTests}" == "debug"
    condition: qbs.buildVariant === "debug"
@endif
    references: [
        "%{JS: '%{TestCaseName}'.toLowerCase()}/%{JS: '%{TestCaseName}'.toLowerCase()}.qbs"
    ]
}
