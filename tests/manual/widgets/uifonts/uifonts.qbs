CppApplication {
    name: "Manual Test Utils UiFonts"
    Depends { name: "Utils" }
    files: "tst_manual_widgets_uifonts.cpp"

    Group {
        name: "studiofonts"
        prefix: "../../../../src/share/3rdparty/studiofonts/"
        files: "studiofonts.qrc"
    }
}

