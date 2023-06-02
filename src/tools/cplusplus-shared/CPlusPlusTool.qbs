import qbs 1.0

QtcTool {
    install: false
    Depends { name: "Qt"; submodules: ["core", "widgets"]; }
    Depends { name: "CPlusPlus" }
    Depends { name: "Utils" }
}
