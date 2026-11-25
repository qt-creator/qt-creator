import qbs

QtcAutotest {
    name: "AiAssistance autotest"
    Depends { name: "Utils" }
    files: "tst_aiassistance.cpp"
}
