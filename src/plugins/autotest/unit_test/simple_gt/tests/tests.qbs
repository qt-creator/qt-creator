import qbs
import qbs.Environment

Project {
    name: "Tests"

    property string googletestDir: Environment.getEnv("GOOGLETEST_DIR")

    references: [
        "gt1/gt1.qbs",
        "gt2/gt2.qbs",
        "gt3/gt3.qbs"
    ]
}
