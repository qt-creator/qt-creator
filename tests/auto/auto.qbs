import qbs

Project {
    name: "QtcAutotests"
    condition: project.withAutotests
    references: [
        "aggregation/aggregation.qbs",
        "algorithm/algorithm.qbs",
        "changeset/changeset.qbs",
        "clangstaticanalyzer/clangstaticanalyzer.qbs",
        "cplusplus/cplusplus.qbs",
        "debugger/debugger.qbs",
        "diff/diff.qbs",
        "environment/environment.qbs",
        "extensionsystem/extensionsystem.qbs",
        "externaltool/externaltool.qbs",
        "filesearch/filesearch.qbs",
        "flamegraph/flamegraph.qbs",
        "generichighlighter/generichighlighter.qbs",
        "json/json.qbs",
        "profilewriter/profilewriter.qbs",
        "qml/qml.qbs",
        "qtcprocess/qtcprocess.qbs",
        "runextensions/runextensions.qbs",
        "sdktool/sdktool.qbs",
        "timeline/timeline.qbs",
        "treeviewfind/treeviewfind.qbs",
        "utils/utils.qbs",
        "valgrind/valgrind.qbs"
    ].concat(project.additionalAutotests)
}
