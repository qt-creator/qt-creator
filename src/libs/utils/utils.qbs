import qbs 1.0
import qbs.FileInfo

Project {
    name: "Utils"

    QtcDevHeaders { }

    QtcLibrary {

        cpp.defines: base.concat([
            "UTILS_LIBRARY",
            "QTC_REL_TOOLS_PATH=\"" + FileInfo.relativePath('/' + qtc.ide_bin_path,
                                                            '/' + qtc.ide_libexec_path) + "\""
        ])
        cpp.dynamicLibraries: {
            var libs = [];
            if (qbs.targetOS.contains("windows")) {
                libs.push("user32", "iphlpapi", "ws2_32", "shell32");
            } else if (qbs.targetOS.contains("unix")) {
                if (!qbs.targetOS.contains("macos"))
                    libs.push("X11");
                if (!qbs.targetOS.contains("openbsd"))
                    libs.push("pthread");
            }
            return libs;
        }

        cpp.enableExceptions: true

        Properties {
            condition: qbs.targetOS.contains("macos")
            cpp.frameworks: ["Foundation"]
        }

        Depends { name: "Qt"; submodules: ["concurrent", "network", "qml", "widgets"] }
        Depends { name: "app_version_header" }

        files: [
            "QtConcurrentTools",
            "asconst.h",
            "algorithm.h",
            "annotateditemdelegate.cpp",
            "annotateditemdelegate.h",
            "ansiescapecodehandler.cpp",
            "ansiescapecodehandler.h",
            "appmainwindow.cpp",
            "appmainwindow.h",
            "basetreeview.cpp",
            "basetreeview.h",
            "bracematcher.cpp",
            "bracematcher.h",
            "buildablehelperlibrary.cpp",
            "buildablehelperlibrary.h",
            "categorysortfiltermodel.cpp",
            "categorysortfiltermodel.h",
            "changeset.cpp",
            "changeset.h",
            "checkablemessagebox.cpp",
            "checkablemessagebox.h",
            "classnamevalidatinglineedit.cpp",
            "classnamevalidatinglineedit.h",
            "codegeneration.cpp",
            "codegeneration.h",
            "completinglineedit.cpp",
            "completinglineedit.h",
            "completingtextedit.cpp",
            "completingtextedit.h",
            "consoleprocess.cpp",
            "consoleprocess.h",
            "consoleprocess_p.h",
            "crumblepath.cpp",
            "crumblepath.h",
            "declarationmacros.h",
            "detailsbutton.cpp",
            "detailsbutton.h",
            "detailswidget.cpp",
            "detailswidget.h",
            "dropsupport.cpp",
            "dropsupport.h",
            "elfreader.cpp",
            "elfreader.h",
            "elidinglabel.cpp",
            "elidinglabel.h",
            "environment.cpp",
            "environment.h",
            "environmentdialog.cpp",
            "environmentdialog.h",
            "environmentmodel.cpp",
            "environmentmodel.h",
            "execmenu.cpp",
            "execmenu.h",
            "executeondestruction.h",
            "fadingindicator.cpp",
            "fadingindicator.h",
            "faketooltip.cpp",
            "faketooltip.h",
            "fancylineedit.cpp",
            "fancylineedit.h",
            "fancymainwindow.cpp",
            "fancymainwindow.h",
            "fileinprojectfinder.cpp",
            "fileinprojectfinder.h",
            "filenamevalidatinglineedit.cpp",
            "filenamevalidatinglineedit.h",
            "filesearch.cpp",
            "filesearch.h",
            "filesystemwatcher.cpp",
            "filesystemwatcher.h",
            "fileutils.cpp",
            "fileutils.h",
            "filewizardpage.cpp",
            "filewizardpage.h",
            "filewizardpage.ui",
            "flowlayout.cpp",
            "flowlayout.h",
            "functiontraits.h",
            "guard.cpp",
            "guard.h",
            "historycompleter.cpp",
            "historycompleter.h",
            "hostosinfo.h",
            "hostosinfo.cpp",
            "htmldocextractor.cpp",
            "htmldocextractor.h",
            "icon.cpp",
            "icon.h",
            "itemviews.cpp",
            "itemviews.h",
            "json.cpp",
            "json.h",
            "linecolumnlabel.cpp",
            "linecolumnlabel.h",
            "listutils.h",
            "macroexpander.cpp",
            "macroexpander.h",
            "mapreduce.h",
            "navigationtreeview.cpp",
            "navigationtreeview.h",
            "networkaccessmanager.cpp",
            "networkaccessmanager.h",
            "newclasswidget.cpp",
            "newclasswidget.h",
            "newclasswidget.ui",
            "osspecificaspects.h",
            "outputformat.h",
            "outputformatter.cpp",
            "outputformatter.h",
            "overridecursor.cpp",
            "overridecursor.h",
            "parameteraction.cpp",
            "parameteraction.h",
            "pathchooser.cpp",
            "pathchooser.h",
            "pathlisteditor.cpp",
            "pathlisteditor.h",
            "persistentsettings.cpp",
            "persistentsettings.h",
            "port.cpp",
            "port.h",
            "portlist.cpp",
            "portlist.h",
            "processhandle.cpp",
            "processhandle.h",
            "progressindicator.cpp",
            "progressindicator.h",
            "projectintropage.cpp",
            "projectintropage.h",
            "projectintropage.ui",
            "proxyaction.cpp",
            "proxyaction.h",
            "proxycredentialsdialog.cpp",
            "proxycredentialsdialog.h",
            "proxycredentialsdialog.ui",
            "qtcassert.cpp",
            "qtcassert.h",
            "qtcolorbutton.cpp",
            "qtcolorbutton.h",
            "qtcprocess.cpp",
            "qtcprocess.h",
            "reloadpromptutils.cpp",
            "reloadpromptutils.h",
            "runextensions.cpp",
            "runextensions.h",
            "savedaction.cpp",
            "savedaction.h",
            "savefile.cpp",
            "savefile.h",
            "scopedswap.h",
            "settingsselector.cpp",
            "settingsselector.h",
            "settingsutils.h",
            "shellcommand.cpp",
            "shellcommand.h",
            "shellcommandpage.cpp",
            "shellcommandpage.h",
            "sizedarray.h",
            "smallstring.h",
            "smallstringiterator.h",
            "smallstringio.h",
            "smallstringliteral.h",
            "smallstringlayout.h",
            "smallstringmemory.h",
            "smallstringvector.h",
            "statuslabel.cpp",
            "statuslabel.h",
            "stringutils.cpp",
            "stringutils.h",
            "styledbar.cpp",
            "styledbar.h",
            "stylehelper.cpp",
            "stylehelper.h",
            "synchronousprocess.cpp",
            "synchronousprocess.h",
            "templateengine.cpp",
            "templateengine.h",
            "temporarydirectory.cpp",
            "temporarydirectory.h",
            "temporaryfile.cpp",
            "temporaryfile.h",
            "textfieldcheckbox.cpp",
            "textfieldcheckbox.h",
            "textfieldcombobox.cpp",
            "textfieldcombobox.h",
            "textfileformat.cpp",
            "textfileformat.h",
            "treemodel.cpp",
            "treemodel.h",
            "treeviewcombobox.cpp",
            "treeviewcombobox.h",
            "headerviewstretcher.cpp",
            "headerviewstretcher.h",
            "uncommentselection.cpp",
            "uncommentselection.h",
            "unixutils.cpp",
            "unixutils.h",
            "utils.qrc",
            "utils_global.h",
            "utilsicons.h",
            "utilsicons.cpp",
            "winutils.cpp",
            "winutils.h",
            "wizard.cpp",
            "wizard.h",
            "wizardpage.cpp",
            "wizardpage.h",
            "images/*.png",
        ]

        Group {
            name: "Theme"
            prefix: "theme/"
            files: [
                "theme.cpp",
                "theme.h",
                "theme_p.h",
            ]
        }

        Group {
            name: "Tooltip"
            prefix: "tooltip/"
            files: [
                "effects.h",
                "reuse.h",
                "tips.cpp",
                "tips.h",
                "tooltip.cpp",
                "tooltip.h",
            ]
        }

        Group {
            name: "WindowsUtils"
            condition: qbs.targetOS.contains("windows")
            files: [
                "consoleprocess_win.cpp",
            ]
        }

        Group {
            name: "ConsoleProcess_unix"
            condition: qbs.targetOS.contains("unix")
            files: [
                "consoleprocess_unix.cpp",
            ]
        }

        Group {
            name: "FileUtils_osx"
            condition: qbs.targetOS.contains("macos")
            files: [
                "fileutils_mac.h", "fileutils_mac.mm",
            ]
        }

        Group {
            name: "MimeTypes"
            prefix: "mimetypes/"
            files: [
                "mimedatabase.cpp",
                "mimedatabase.h",
                "mimedatabase_p.h",
                "mimeglobpattern.cpp",
                "mimeglobpattern_p.h",
                "mimemagicrule.cpp",
                "mimemagicrule_p.h",
                "mimemagicrulematcher.cpp",
                "mimemagicrulematcher_p.h",
                "mimeprovider.cpp",
                "mimeprovider_p.h",
                "mimetype.cpp",
                "mimetype.h",
                "mimetype_p.h",
                "mimetypeparser.cpp",
                "mimetypeparser_p.h",
            ]
        }

        Export {
            Depends { name: "Qt"; submodules: ["concurrent", "widgets" ] }
        }
    }
}
