import qbs

QtcAutotest {
    name: "DialogTask autotest"
    Depends { name: "Utils" }
    files: "tst_dialogtask.cpp"
}
