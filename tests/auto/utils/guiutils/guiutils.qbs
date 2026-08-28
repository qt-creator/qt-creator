import qbs

QtcAutotest {
    name: "GuiUtils autotest"
    Depends { name: "Utils" }
    files: "tst_guiutils.cpp"
}
