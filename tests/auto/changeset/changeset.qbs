import qbs
import "../autotest.qbs" as Autotest

Autotest {
    name: "ChangeSet autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.gui" } // TODO: Remove once qbs bug is fixed.
    files: "tst_changeset.cpp"
}
