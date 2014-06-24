import qbs 1.0
import QtcLibrary

QtcLibrary {
    name: "Utils"

    cpp.defines: base.concat("QTCREATOR_UTILS_LIB")

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: [
            "user32",
            "iphlpapi",
            "ws2_32",
            "shell32",
        ]
    }
    Properties {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx")
        cpp.dynamicLibraries: ["X11"]
    }

    Depends { name: "Qt"; submodules: ["widgets", "network", "script", "concurrent"] }
    Depends { name: "app_version_header" }

    files: [
        "QtConcurrentTools",
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
        "detailsbutton.cpp",
        "detailsbutton.h",
        "detailswidget.cpp",
        "detailswidget.h",
        "elfreader.cpp",
        "elfreader.h",
        "elidinglabel.cpp",
        "elidinglabel.h",
        "environment.cpp",
        "environment.h",
        "environmentmodel.cpp",
        "environmentmodel.h",
        "execmenu.cpp",
        "execmenu.h",
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
        "function.cpp",
        "function.h",
        "historycompleter.cpp",
        "historycompleter.h",
        "hostosinfo.h",
        "hostosinfo.cpp",
        "htmldocextractor.cpp",
        "htmldocextractor.h",
        "ipaddresslineedit.cpp",
        "ipaddresslineedit.h",
        "itemviews.cpp",
        "itemviews.h",
        "iwelcomepage.cpp",
        "iwelcomepage.h",
        "json.cpp",
        "json.h",
        "linecolumnlabel.cpp",
        "linecolumnlabel.h",
        "listutils.h",
        "logging.h",
        "multitask.h",
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
        "parameteraction.cpp",
        "parameteraction.h",
        "pathchooser.cpp",
        "pathchooser.h",
        "pathlisteditor.cpp",
        "pathlisteditor.h",
        "persistentsettings.cpp",
        "persistentsettings.h",
        "portlist.cpp",
        "portlist.h",
        "projectintropage.cpp",
        "projectintropage.h",
        "projectintropage.ui",
        "projectnamevalidatinglineedit.cpp",
        "projectnamevalidatinglineedit.h",
        "proxyaction.cpp",
        "proxyaction.h",
        "qtcassert.cpp",
        "qtcassert.h",
        "qtcolorbutton.cpp",
        "qtcolorbutton.h",
        "qtcprocess.cpp",
        "qtcprocess.h",
        "reloadpromptutils.cpp",
        "reloadpromptutils.h",
        "runextensions.h",
        "savedaction.cpp",
        "savedaction.h",
        "savefile.cpp",
        "savefile.h",
        "scopedswap.h",
        "settingsselector.cpp",
        "settingsselector.h",
        "settingsutils.h",
        "sleep.cpp",
        "sleep.h",
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
        "tcpportsgatherer.cpp",
        "tcpportsgatherer.h",
        "textfileformat.cpp",
        "textfileformat.h",
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
        "winutils.cpp",
        "winutils.h",
        "wizard.cpp",
        "wizard.h",
        "images/arrow.png",
        "images/crumblepath-segment-end.png",
        "images/crumblepath-segment-hover-end.png",
        "images/crumblepath-segment-hover.png",
        "images/crumblepath-segment-selected-end.png",
        "images/crumblepath-segment-selected.png",
        "images/crumblepath-segment.png",
        "images/triangle_vert.png",
    ]

    Group {
        name: "Tooltip"
        prefix: "tooltip/"
        files: [
            "effects.h",
            "reuse.h",
            "tipcontents.cpp",
            "tipcontents.h",
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

    Export {
        Depends { name: "Qt"; submodules: ["concurrent", "widgets" ] }
    }
}

