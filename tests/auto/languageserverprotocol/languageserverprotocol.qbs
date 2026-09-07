import qbs

QtcAutotest {
    name: "LanguageServerProtocol autotest"
    Depends { name: "LanguageServerProtocol" }
    files: "tst_languageserverprotocol.cpp"
}
