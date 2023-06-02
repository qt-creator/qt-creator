import qbs
import qbs.File

Project {
    name: "Libs"
    property string qlitehtmlQbs: path + "/qlitehtml/src/qlitehtml.qbs"
    property stringList qlitehtml: File.exists(qlitehtmlQbs) ? [qlitehtmlQbs] : []
    references: [
        "advanceddockingsystem/advanceddockingsystem.qbs",
        "aggregation/aggregation.qbs",
        "cplusplus/cplusplus.qbs",
        "extensionsystem/extensionsystem.qbs",
        "glsl/glsl.qbs",
        "languageserverprotocol/languageserverprotocol.qbs",
        "languageutils/languageutils.qbs",
        "modelinglib/modelinglib.qbs",
        "nanotrace/nanotrace.qbs",
        "qmleditorwidgets/qmleditorwidgets.qbs",
        "qmljs/qmljs.qbs",
        "qmldebug/qmldebug.qbs",
        "qtcreatorcdbext/qtcreatorcdbext.qbs",
        "solutions/solutions.qbs",
        "sqlite/sqlite.qbs",
        "tracing/tracing.qbs",
        "utils/process_ctrlc_stub.qbs",
        "utils/utils.qbs",
        "3rdparty/libptyqt/ptyqt.qbs",
        "3rdparty/libvterm/vterm.qbs",
        "3rdparty/syntax-highlighting/syntax-highlighting.qbs",
        "3rdparty/winpty/winpty.qbs",
        "3rdparty/yaml-cpp/yaml-cpp.qbs",
    ].concat(qlitehtml).concat(project.additionalLibs)
}
