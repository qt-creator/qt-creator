import qbs

QtcAutotest {
    name: "SSH autotest"
    Depends { name: "QtcSsh" }
    files: "tst_ssh.cpp"
}
