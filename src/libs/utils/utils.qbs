import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "Utils"

    cpp.defines: ["QTCREATOR_UTILS_LIB"]
    cpp.includePaths: [
        ".",
        "..",
        "../..",
        buildDirectory
    ]

    Properties {
        condition: qbs.targetOS == "windows"
        cpp.dynamicLibraries: [
            "user32",
            "iphlpapi",
            "ws2_32"
        ]
    }
    Properties {
        condition: qbs.targetOS == "linux"
        cpp.dynamicLibraries: ["X11"]
    }

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["widgets", "network", "script", "concurrent"] }
    Depends { name: "app_version_header" }

    files: [
        "filewizardpage.ui",
        "newclasswidget.ui",
        "projectintropage.ui",
        "utils.qrc",
        "annotateditemdelegate.cpp",
        "annotateditemdelegate.h",
        "basetreeview.cpp",
        "basetreeview.h",
        "basevalidatinglineedit.cpp",
        "basevalidatinglineedit.h",
        "bracematcher.cpp",
        "bracematcher.h",
        "buildablehelperlibrary.h",
        "changeset.cpp",
        "changeset.h",
        "classnamevalidatinglineedit.cpp",
        "classnamevalidatinglineedit.h",
        "codegeneration.cpp",
        "codegeneration.h",
        "completingtextedit.cpp",
        "completingtextedit.h",
        "consoleprocess.cpp",
        "consoleprocess.h",
        "consoleprocess_p.h",
        "crumblepath.h",
        "detailsbutton.cpp",
        "detailsbutton.h",
        "detailswidget.cpp",
        "detailswidget.h",
        "elfreader.cpp",
        "elfreader.h",
        "environment.h",
        "environmentmodel.cpp",
        "environmentmodel.h",
        "faketooltip.cpp",
        "faketooltip.h",
        "fancylineedit.cpp",
        "fancylineedit.h",
        "fancymainwindow.cpp",
        "fancymainwindow.h",
        "appmainwindow.h",
        "appmainwindow.cpp",
        "fileinprojectfinder.cpp",
        "fileinprojectfinder.h",
        "filenamevalidatinglineedit.h",
        "filesearch.cpp",
        "filesearch.h",
        "filesystemwatcher.cpp",
        "filesystemwatcher.h",
        "fileutils.h",
        "filewizarddialog.cpp",
        "filewizarddialog.h",
        "filewizardpage.cpp",
        "filewizardpage.h",
        "filterlineedit.cpp",
        "filterlineedit.h",
        "flowlayout.cpp",
        "flowlayout.h",
        "historycompleter.h",
        "htmldocextractor.cpp",
        "htmldocextractor.h",
        "ipaddresslineedit.h",
        "iwelcomepage.cpp",
        "iwelcomepage.h",
        "json.cpp",
        "json.h",
        "linecolumnlabel.cpp",
        "linecolumnlabel.h",
        "listutils.h",
        "navigationtreeview.cpp",
        "navigationtreeview.h",
        "networkaccessmanager.h",
        "newclasswidget.cpp",
        "newclasswidget.h",
        "outputformat.h",
        "outputformatter.cpp",
        "outputformatter.h",
        "parameteraction.cpp",
        "parameteraction.h",
        "pathchooser.cpp",
        "pathchooser.h",
        "pathlisteditor.h",
        "projectintropage.cpp",
        "projectintropage.h",
        "projectnamevalidatinglineedit.cpp",
        "projectnamevalidatinglineedit.h",
        "proxyaction.h",
        "qtcassert.h",
        "qtcassert.cpp",
        "qtcolorbutton.cpp",
        "qtcolorbutton.h",
        "qtcprocess.h",
        "reloadpromptutils.cpp",
        "reloadpromptutils.h",
        "savedaction.h",
        "savefile.cpp",
        "savefile.h",
        "settingsselector.cpp",
        "settingsutils.h",
        "statuslabel.cpp",
        "statuslabel.h",
        "stringutils.cpp",
        "stringutils.h",
        "styledbar.cpp",
        "styledbar.h",
        "stylehelper.h",
        "submiteditorwidget.cpp",
        "submiteditorwidget.h",
        "submiteditorwidget.ui",
        "submitfieldwidget.cpp",
        "submitfieldwidget.h",
        "synchronousprocess.cpp",
        "synchronousprocess.h",
        "tcpportsgatherer.cpp",
        "tcpportsgatherer.h",
        "textfileformat.cpp",
        "textfileformat.h",
        "treewidgetcolumnstretcher.cpp",
        "treewidgetcolumnstretcher.h",
        "uncommentselection.cpp",
        "uncommentselection.h",
        "utils_global.h",
        "wizard.cpp",
        "wizard.h",
        "hostosinfo.h",
        "persistentsettings.h",
        "settingsselector.h",
        "buildablehelperlibrary.cpp",
        "checkablemessagebox.cpp",
        "checkablemessagebox.h",
        "crumblepath.cpp",
        "environment.cpp",
        "filenamevalidatinglineedit.cpp",
        "fileutils.cpp",
        "historycompleter.cpp",
        "ipaddresslineedit.cpp",
        "networkaccessmanager.cpp",
        "pathlisteditor.cpp",
        "persistentsettings.cpp",
        "portlist.cpp",
        "portlist.h",
        "proxyaction.cpp",
        "qtcprocess.cpp",
        "savedaction.cpp",
        "stylehelper.cpp",
        "multitask.h",
        "runextensions.h",
        "images/arrow.png",
        "images/crumblepath-segment-end.png",
        "images/crumblepath-segment-hover-end.png",
        "images/crumblepath-segment-hover.png",
        "images/crumblepath-segment-selected-end.png",
        "images/crumblepath-segment-selected.png",
        "images/crumblepath-segment.png",
        "images/removesubmitfield.png",
        "images/triangle_vert.png",
    ]

    Group {
        condition: qbs.targetOS == "windows"
        files: [
            "consoleprocess_win.cpp",
            "winutils.cpp",
            "winutils.h",
        ]
    }

    Group {
        condition: qbs.targetOS == "linux" || qbs.targetOS == "mac"
        files: [
            "consoleprocess_unix.cpp",
        ]
    }

    Group {
        condition: qbs.targetOS == "linux"
        files: [
            "unixutils.h",
            "unixutils.cpp"
        ]
    }

    ProductModule {
        Depends { name: "Qt"; submodules: ["concurrent", "widgets" ] }
    }
}

