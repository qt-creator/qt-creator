import qbs

QtcAutotest {
    name: "AsyncTask autotest"
    Depends { name: "Utils" }
    files: "tst_asynctask.cpp"
}
