import qbs

Project {
    name: "Libs"
    references: [
        "aggregation/aggregation.qbs",
        "cplusplus/cplusplus.qbs",
        "extensionsystem/extensionsystem.qbs",
        "glsl/glsl.qbs",
        "languageutils/languageutils.qbs",
        "qmleditorwidgets/qmleditorwidgets.qbs",
        "qmljs/qmljs.qbs",
        "qmldebug/qmldebug.qbs",
        "qtcomponents/styleitem/styleitem.qbs",
        "ssh/ssh.qbs",
        "utils/process_stub.qbs",
        "utils/process_ctrlc_stub.qbs",
        "utils/utils.qbs",
        "zeroconf/zeroconf.qbs",
    ]
}
