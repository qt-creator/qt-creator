import qbs

Project {
    name: "ExtensionSystem cirular plugins autotests"
    references: [
        "plugin1/plugin1.qbs",
        "plugin2/plugin2.qbs",
        "plugin3/plugin3.qbs"
    ]
}
