import qbs

QtcAutotest {
    name: "Acp autotest"
    Depends { name: "AcpLib" }
    files: "tst_acp.cpp"
}
