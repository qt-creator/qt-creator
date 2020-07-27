import qbs
import "../testapp.qbs" as TestApp

TestApp {
    testName: "leak1"
    Depends { name: "Qt.core" }
}
