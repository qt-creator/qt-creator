import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "Utils"

    cpp.defines: base.concat("QTCREATOR_UTILS_LIB")

    Properties {
        condition: qbs.targetOS == "windows"
        cpp.dynamicLibraries: [
            "user32",
            "iphlpapi",
            "ws2_32"
        ]
    }
    Properties {
        condition: qbs.targetPlatform.indexOf("unix") != -1 && qbs.targetOS != "mac"
        cpp.dynamicLibraries: ["X11"]
    }

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["widgets", "network", "script", "concurrent"] }
    Depends { name: "app_version_header" }

    files: [
        "annotateditemdelegate.cpp",
        "annotateditemdelegate.h",
        "appmainwindow.cpp",
        "appmainwindow.h",
        "basetreeview.cpp",
        "basetreeview.h",
        "basevalidatinglineedit.cpp",
        "basevalidatinglineedit.h",
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
        "filewizarddialog.cpp",
        "filewizarddialog.h",
        "filewizardpage.cpp",
        "filewizardpage.h",
        "filewizardpage.ui",
        "filterlineedit.cpp",
        "filterlineedit.h",
        "flowlayout.cpp",
        "flowlayout.h",
        "historycompleter.cpp",
        "historycompleter.h",
        "hostosinfo.h",
        "hostosinfo.cpp",
        "htmldocextractor.cpp",
        "htmldocextractor.h",
        "ipaddresslineedit.cpp",
        "ipaddresslineedit.h",
        "iwelcomepage.cpp",
        "iwelcomepage.h",
        "json.cpp",
        "json.h",
        "linecolumnlabel.cpp",
        "linecolumnlabel.h",
        "listutils.h",
        "multitask.h",
        "navigationtreeview.cpp",
        "navigationtreeview.h",
        "networkaccessmanager.cpp",
        "networkaccessmanager.h",
        "newclasswidget.cpp",
        "newclasswidget.h",
        "newclasswidget.ui",
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
        "settingsselector.cpp",
        "settingsselector.h",
        "settingsutils.h",
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
        "headerviewstretcher.cpp",
        "headerviewstretcher.h",
        "uncommentselection.cpp",
        "uncommentselection.h",
        "unixutils.cpp",
        "unixutils.h",
        "utils.qrc",
        "utils_global.h",
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
        condition: qbs.targetOS == "windows"
        files: [
            "consoleprocess_win.cpp",
            "winutils.cpp",
            "winutils.h",
        ]
    }

    Group {
        condition: qbs.targetPlatform.indexOf("unix") != -1
        files: [
            "consoleprocess_unix.cpp",
        ]
    }

    ProductModule {
        // ### [ remove, once qbs supports merging of ProductModule items in derived products
        Depends { name: "cpp" }
        cpp.includePaths: [ ".." ]
        // ### ]
        Depends { name: "Qt"; submodules: ["concurrent", "widgets" ] }
    }
}

