import qbs

QtcAutotest {
    name: "SSH autotest"
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }
    files: "tst_ssh.cpp"
}
