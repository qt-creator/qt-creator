import qbs

Project {
    name: "Libs"
    references: [
        "aggregation/aggregation.qbs",
        "clangsupport/clangsupport.qbs",
        "cplusplus/cplusplus.qbs",
        "extensionsystem/extensionsystem.qbs",
        "flamegraph/flamegraph.qbs",
        "glsl/glsl.qbs",
        "languageutils/languageutils.qbs",
        "modelinglib/modelinglib.qbs",
        "qmleditorwidgets/qmleditorwidgets.qbs",
        "qmljs/qmljs.qbs",
        "qmldebug/qmldebug.qbs",
        "qtcreatorcdbext/qtcreatorcdbext.qbs",
        "sqlite/sqlite.qbs",
        "ssh/ssh.qbs",
        "timeline/timeline.qbs",
        "utils/process_stub.qbs",
        "utils/process_ctrlc_stub.qbs",
        "utils/utils.qbs",
    ].concat(project.additionalLibs)
}
