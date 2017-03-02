import qbs

QtcAutotest {
    name: "ObjectPool autotest"
    Depends { name: "Utils" }
    files: "tst_objectpool.cpp"
}
