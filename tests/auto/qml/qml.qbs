import qbs

Project {
    name: "QML autotests"
    references: [
        "codemodel/codemodel.qbs",
//        "connectioneditor/connectioneditor.qbs",
//        "qmldesigner/qmldesigner.qbs",
        "qmleditor/qmleditor.qbs",
        "qmljssimplereader/qmljssimplereader.qbs",
        "qmljsutils/qmljsutils.qbs",
        "qrcparser/qrcparser.qbs",
        "reformatter/reformatter.qbs",
        "persistenttrie/persistenttrie.qbs"
    ]
}
