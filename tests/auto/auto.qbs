import qbs

Project {
    name: "QtcAutotests"
    condition: project.withAutotests
    references: [
        "aggregation/aggregation.qbs",
        "changeset/changeset.qbs",
        "cplusplus/cplusplus.qbs",
        "debugger/debugger.qbs",
        "diff/diff.qbs",
        "environment/environment.qbs",
        "extensionsystem/extensionsystem.qbs",
        "externaltool/externaltool.qbs",
        "filesearch/filesearch.qbs",
        "generichighlighter/generichighlighter.qbs",
        "ioutils/ioutils.qbs",
        "profilewriter/profilewriter.qbs",
        "qml/qml.qbs",
        "qtcprocess/qtcprocess.qbs",
        "sdktool/sdktool.qbs",
        "timeline/timeline.qbs",
        "treeviewfind/treeviewfind.qbs",
        "utils/utils.qbs",
        "valgrind/valgrind.qbs"
    ].concat(project.additionalAutotests)
}
