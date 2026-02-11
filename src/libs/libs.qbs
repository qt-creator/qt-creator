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
        "devcontainer/devcontainer.qbs",
        "extensionsystem/extensionsystem.qbs",
        "glsl/glsl.qbs",
        "gocmdbridge/gocmdbridge.qbs",
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
        "QtTaskTree/QtTaskTree/QtTaskTree.qbs",
        "3rdparty/bzip2/bzip2.qbs",
        "3rdparty/libarchive/libarchive.qbs",
        "3rdparty/libptyqt/ptyqt.qbs",
        "3rdparty/libvterm/vterm.qbs",
        "3rdparty/lua/lua.qbs",
        "3rdparty/qtkeychain/qtkeychain.qbs",
        "3rdparty/sol2/sol2.qbs",
        "3rdparty/syntax-highlighting/syntax-highlighting.qbs",
        "3rdparty/ui_watchdog/uiwatchdog.qbs",
        "3rdparty/xz/xz.qbs",
        "3rdparty/winpty/winpty.qbs",
        "3rdparty/yaml-cpp/yaml-cpp.qbs",
        "3rdparty/zlib",
    ].concat(qlitehtml).concat(project.additionalLibs)
}
