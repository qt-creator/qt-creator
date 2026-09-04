import qbs

QtcAutotest {
    name: "SpellChecker autotest"
    Depends { name: "Utils" }
    files: "tst_spellchecker.cpp"
}
