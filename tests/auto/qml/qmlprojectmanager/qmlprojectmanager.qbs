import qbs

Project {
    condition: false // Known to be broken, nobody cares.
    name: "QmlProjectManager autotests"
    references: "fileformat/fileformat.qbs"
}
