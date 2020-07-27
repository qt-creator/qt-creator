import qbs
import "../testapp.qbs" as TestApp

TestApp {
    testName: "leak4"
    Depends { name: "Qt.core" }
}
