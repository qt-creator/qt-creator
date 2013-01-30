import qbs.base 1.0
import qbs.fileinfo 1.0 as FileInfo
import "qbs/defaults.js" as Defaults

Project {
    property string ide_version_major: '2'
    property string ide_version_minor: '6'
    property string ide_version_release: '81'
    property string qtcreator_version: ide_version_major + '.' + ide_version_minor + '.' + ide_version_release
    moduleSearchPaths: "qbs"

    references: [
        "lib/qtcreator/qtcomponents/qtcomponents.qbs",
        "share/share.qbs",
        "share/qtcreator/translations/translations.qbs",
        "src/libs/aggregation/aggregation.qbs",
        "src/libs/cplusplus/cplusplus.qbs",
        "src/libs/extensionsystem/extensionsystem.qbs",
        "src/libs/glsl/glsl.qbs",
        "src/libs/languageutils/languageutils.qbs",
        "src/libs/qmleditorwidgets/qmleditorwidgets.qbs",
        "src/libs/qmljs/qmljs.qbs",
        "src/libs/qmldebug/qmldebug.qbs",
        "src/libs/qtcomponents/styleitem/styleitem.qbs",
        "src/libs/ssh/ssh.qbs",
        "src/libs/utils/process_stub.qbs",
        "src/libs/utils/process_ctrlc_stub.qbs",
        "src/libs/utils/utils.qbs",
        "src/libs/zeroconf/zeroconf.qbs",
        "src/plugins/analyzerbase/analyzerbase.qbs",
        "src/plugins/android/android.qbs",
        "src/plugins/autotoolsprojectmanager/autotoolsprojectmanager.qbs",
        "src/plugins/bazaar/bazaar.qbs",
        "src/plugins/bineditor/bineditor.qbs",
        "src/plugins/bookmarks/bookmarks.qbs",
        "src/plugins/classview/classview.qbs",
        "src/plugins/clearcase/clearcase.qbs",
        "src/plugins/cmakeprojectmanager/cmakeprojectmanager.qbs",
        "src/plugins/coreplugin/coreplugin.qbs",
        "src/plugins/coreplugin/images/logo/logo.qbs",
        "src/plugins/cpaster/cpaster.qbs",
        "src/plugins/cppeditor/cppeditor.qbs",
        "src/plugins/cpptools/cpptools.qbs",
        "src/plugins/cvs/cvs.qbs",
        "src/plugins/debugger/debugger.qbs",
        "src/plugins/debugger/ptracepreload.qbs",
        "src/plugins/designer/designer.qbs",
        "src/plugins/fakevim/fakevim.qbs",
        "src/plugins/find/find.qbs",
        "src/plugins/genericprojectmanager/genericprojectmanager.qbs",
        "src/plugins/git/git.qbs",
        "src/plugins/glsleditor/glsleditor.qbs",
        "src/plugins/helloworld/helloworld.qbs",
        "src/plugins/help/help.qbs",
        "src/plugins/imageviewer/imageviewer.qbs",
        "src/plugins/locator/locator.qbs",
        "src/plugins/macros/macros.qbs",
        "src/plugins/madde/madde.qbs",
        "src/plugins/mercurial/mercurial.qbs",
        "src/plugins/perforce/perforce.qbs",
        "src/plugins/projectexplorer/projectexplorer.qbs",
        "src/plugins/qbsprojectmanager/qbsprojectmanager.qbs",
//        "src/plugins/qmldesigner/qmldesigner.qbs",
        "src/plugins/qmljseditor/qmljseditor.qbs",
        "src/plugins/qmljstools/qmljstools.qbs",
        "src/plugins/qmlprofiler/qmlprofiler.qbs",
        "src/plugins/qmlprojectmanager/qmlprojectmanager.qbs",
        "src/plugins/qnx/qnx.qbs",
        "src/plugins/qt4projectmanager/qt4projectmanager.qbs",
        "src/plugins/qtsupport/qtsupport.qbs",
        "src/plugins/remotelinux/remotelinux.qbs",
        "src/plugins/resourceeditor/resourceeditor.qbs",
        "src/plugins/subversion/subversion.qbs",
        "src/plugins/tasklist/tasklist.qbs",
        "src/plugins/texteditor/texteditor.qbs",
        "src/plugins/todo/todo.qbs",
        "src/plugins/updateinfo/updateinfo.qbs",
        "src/plugins/valgrind/valgrind.qbs",
        "src/plugins/vcsbase/vcsbase.qbs",
        "src/plugins/welcome/welcome.qbs",
        "src/tools/qtcdebugger/qtcdebugger.qbs",
        "src/tools/qtcreatorcrashhandler/qtcreatorcrashhandler.qbs",
        "src/tools/qtpromaker/qtpromaker.qbs",
        "src/plugins/cpaster/frontend/frontend.qbs",
        "src/tools/sdktool/sdktool.qbs"
    ]

    Product {
        name: "app_version_header"
        type: "hpp"
        files: "src/app/app_version.h.in"
        property string ide_version_major: project.ide_version_major
        property string ide_version_minor: project.ide_version_minor
        property string ide_version_release: project.ide_version_release
        property string qtcreator_version: project.qtcreator_version

        Transformer {
            inputs: ["src/app/app_version.h.in"]
            Artifact {
                fileName: "app/app_version.h"
                fileTags: "hpp"
            }
            prepare: {
                var cmd = new JavaScriptCommand();
                cmd.description = "generating app_version.h";
                cmd.highlight = "codegen";
                cmd.qtcreator_version = product.qtcreator_version;
                cmd.ide_version_major = product.ide_version_major;
                cmd.ide_version_minor = product.ide_version_minor;
                cmd.ide_version_release = product.ide_version_release;
                cmd.onWindows = (product.modules.qbs.targetOS == "windows");
                cmd.sourceCode = function() {
                    var file = new TextFile(input.fileName);
                    var content = file.readAll();
                    // replace quoted quotes
                    content = content.replace(/\\\"/g, '"');
                    // replace Windows line endings
                    if (onWindows)
                        content = content.replace(/\r\n/g, "\n");
                    // replace the magic qmake incantations
                    content = content.replace(/(\n#define IDE_VERSION) .+\n/, "$1 " + qtcreator_version + "\n");
                    content = content.replace(/(\n#define IDE_VERSION_MAJOR) .+\n/, "$1 " + ide_version_major + "\n")
                    content = content.replace(/(\n#define IDE_VERSION_MINOR) .+\n/, "$1 " + ide_version_minor + "\n")
                    content = content.replace(/(\n#define IDE_VERSION_RELEASE) .+\n/, "$1 " + ide_version_release + "\n")
                    file = new TextFile(output.fileName, TextFile.WriteOnly);
                    file.truncate();
                    file.write(content);
                    file.close();
                }
                return cmd;
            }
        }

        ProductModule {
            Depends { name: "cpp" }
            cpp.includePaths: product.buildDirectory
        }
    }

    Application {
        name: "qtcreator"
        consoleApplication: qbs.debugInformation

        cpp.rpaths: ["$ORIGIN/../lib/qtcreator"]
        cpp.defines: Defaults.defines(qbs)
        cpp.linkerFlags: {
            if (qbs.buildVariant == "release" && (qbs.toolchain == "gcc" || qbs.toolchain == "mingw"))
                return ["-Wl,-s"]
        }
        cpp.includePaths: [
            "src",
            "src/libs",
            "src/shared/qtsingleapplication",
            "src/shared/qtlockedfile",
            buildDirectory
        ]

        Depends { name: "app_version_header" }
        Depends { name: "cpp" }
        Depends { name: "Qt"; submodules: ["widgets", "network"] }
        Depends { name: "Utils" }
        Depends { name: "ExtensionSystem" }

        files: [
            "src/app/main.cpp",
            "src/app/qtcreator.rc",
            "src/shared/qtsingleapplication/qtsingleapplication.h",
            "src/shared/qtsingleapplication/qtsingleapplication.cpp",
            "src/shared/qtsingleapplication/qtlocalpeer.h",
            "src/shared/qtsingleapplication/qtlocalpeer.cpp",
            "src/shared/qtlockedfile/qtlockedfile.cpp",
            "src/tools/qtcreatorcrashhandler/crashhandlersetup.cpp",
            "src/tools/qtcreatorcrashhandler/crashhandlersetup.h"
        ]

        Group {
            condition: qbs.targetOS == "linux" || qbs.targetOS == "macx"
            files: "bin/qtcreator.sh"
            qbs.install: true
            qbs.installDir: "bin"
        }

        Group {
           condition: qbs.targetOS == "linux" || qbs.targetOS == "macx"
           files: [
               "src/shared/qtlockedfile/qtlockedfile_unix.cpp"
           ]
        }

        Group {
           condition: qbs.targetOS == "windows"
           files: [
               "src/shared/qtlockedfile/qtlockedfile_win.cpp"
           ]
        }

        Group {
            fileTagsFilter: product.type
            qbs.install: true
            qbs.installDir: "bin"
        }
    }
}
