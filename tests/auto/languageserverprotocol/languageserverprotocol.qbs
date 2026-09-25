import qbs

QtcAutotest {
    name: "LanguageServerProtocol autotest"
    Depends { name: "LanguageServerProtocol" }
    Depends { name: "Utils" }
    files: "tst_languageserverprotocol.cpp"
}
