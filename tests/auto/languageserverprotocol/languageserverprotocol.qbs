import qbs

QtcAutotest {
    name: "Language Server Protocol autotest"
    Depends { name: "Utils" }
    Depends { name: "LanguageServerProtocol" }
    files: "tst_languageserverprotocol.cpp"
}
