import qbs

Project {
    name: "QML designer projects"
    references: [
        "qmldesignerplugin.qbs",
        "qtquickplugin/qtquickplugin.qbs",
        "componentsplugin/componentsplugin.qbs",
        "qmlpreviewplugin/qmlpreviewplugin.qbs",
        "assetexporterplugin/assetexporterplugin.qbs"
    ]
}
