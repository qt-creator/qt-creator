import qbs
import qbs.Utilities

QtcAutotest {
    name: "DevContainer autotest"
    Depends { name: "DevContainer" }
    Depends { name: "Utils" }
    Depends { name: "Qt.gui" }

    condition: !qbs.toolchain.contains("msvc") || Utilities.versionCompare(cpp.compilerVersion, "19.30.0") > 0

    files: "tst_devcontainer.cpp"
}
