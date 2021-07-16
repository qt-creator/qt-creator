import qbs

QtcAutotest {
    name: "IndexedContainerProxyConstIterator autotest"
    Depends { name: "Utils" }
    files: "tst_indexedcontainerproxyconstiterator.cpp"
}
