import qbs

QtcAutotest {
    name: "FileInProjectFinder autotest"
    Depends { name: "Utils" }
    files: "tst_fileinprojectfinder.cpp"
}
