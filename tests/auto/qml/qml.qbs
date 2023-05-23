import qbs

Project {
    name: "QML autotests"
    references: [
        "codemodel/codemodel.qbs",
//        "qmldesigner/qmldesigner.qbs",
        "qmleditor/qmleditor.qbs",
        "qmljssimplereader/qmljssimplereader.qbs",
        "qmlprojectmanager/qmlprojectmanager.qbs",
        "qrcparser/qrcparser.qbs",
        "reformatter/reformatter.qbs",
        "persistenttrie/persistenttrie.qbs"
    ]
}
