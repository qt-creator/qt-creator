import qbs 1.0

QtcTool {
    Depends { name: "Qt"; submodules: ["core", "widgets"]; }
    Depends { name: "CPlusPlus" }
    Depends { name: "Utils" }
}
