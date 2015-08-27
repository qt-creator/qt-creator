import qbs

QtcAutotest {
    name: "ChangeSet autotest"
    Depends { name: "Utils" }
    files: "tst_changeset.cpp"
}
