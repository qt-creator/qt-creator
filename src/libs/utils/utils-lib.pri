shared {
    DEFINES += UTILS_LIBRARY
} else {
    DEFINES += QTCREATOR_UTILS_STATIC_LIB
}

!win32:{
    isEmpty(IDE_LIBEXEC_PATH) | isEmpty(IDE_BIN_PATH): {
        warning("using utils-lib.pri without IDE_LIBEXEC_PATH or IDE_BIN_PATH results in empty QTC_REL_TOOLS_PATH")
        DEFINES += QTC_REL_TOOLS_PATH=$$shell_quote(\"\")
    } else {
        DEFINES += QTC_REL_TOOLS_PATH=$$shell_quote(\"$$relative_path($$IDE_LIBEXEC_PATH, $$IDE_BIN_PATH)\")
    }
}

QT += widgets gui network qml

CONFIG += exceptions # used by portlist.cpp, textfileformat.cpp, and ssh/*

win32: LIBS += -luser32 -lshell32
# PortsGatherer
win32: LIBS += -liphlpapi -lws2_32

SOURCES += $$PWD/environment.cpp \
    $$PWD/environmentmodel.cpp \
    $$PWD/environmentdialog.cpp \
    $$PWD/qtcprocess.cpp \
    $$PWD/reloadpromptutils.cpp \
    $$PWD/shellcommand.cpp \
    $$PWD/shellcommandpage.cpp \
    $$PWD/settingsselector.cpp \
    $$PWD/stringutils.cpp \
    $$PWD/templateengine.cpp \
    $$PWD/temporarydirectory.cpp \
    $$PWD/temporaryfile.cpp \
    $$PWD/textfieldcheckbox.cpp \
    $$PWD/textfieldcombobox.cpp \
    $$PWD/filesearch.cpp \
    $$PWD/pathchooser.cpp \
    $$PWD/pathlisteditor.cpp \
    $$PWD/wizard.cpp \
    $$PWD/wizardpage.cpp \
    $$PWD/filewizardpage.cpp \
    $$PWD/filesystemwatcher.cpp \
    $$PWD/projectintropage.cpp \
    $$PWD/filenamevalidatinglineedit.cpp \
    $$PWD/codegeneration.cpp \
    $$PWD/newclasswidget.cpp \
    $$PWD/classnamevalidatinglineedit.cpp \
    $$PWD/linecolumnlabel.cpp \
    $$PWD/fancylineedit.cpp \
    $$PWD/qtcolorbutton.cpp \
    $$PWD/savedaction.cpp \
    $$PWD/synchronousprocess.cpp \
    $$PWD/savefile.cpp \
    $$PWD/fileutils.cpp \
    $$PWD/textfileformat.cpp \
    $$PWD/consoleprocess.cpp \
    $$PWD/uncommentselection.cpp \
    $$PWD/parameteraction.cpp \
    $$PWD/headerviewstretcher.cpp \
    $$PWD/checkablemessagebox.cpp \
    $$PWD/styledbar.cpp \
    $$PWD/stylehelper.cpp \
    $$PWD/fancymainwindow.cpp \
    $$PWD/detailsbutton.cpp \
    $$PWD/detailswidget.cpp \
    $$PWD/changeset.cpp \
    $$PWD/faketooltip.cpp \
    $$PWD/htmldocextractor.cpp \
    $$PWD/navigationtreeview.cpp \
    $$PWD/crumblepath.cpp \
    $$PWD/historycompleter.cpp \
    $$PWD/buildablehelperlibrary.cpp \
    $$PWD/annotateditemdelegate.cpp \
    $$PWD/fileinprojectfinder.cpp \
    $$PWD/statuslabel.cpp \
    $$PWD/outputformatter.cpp \
    $$PWD/flowlayout.cpp \
    $$PWD/networkaccessmanager.cpp \
    $$PWD/persistentsettings.cpp \
    $$PWD/completingtextedit.cpp \
    $$PWD/json.cpp \
    $$PWD/portlist.cpp \
    $$PWD/processhandle.cpp \
    $$PWD/appmainwindow.cpp \
    $$PWD/basetreeview.cpp \
    $$PWD/qtcassert.cpp \
    $$PWD/elfreader.cpp \
    $$PWD/bracematcher.cpp \
    $$PWD/proxyaction.cpp \
    $$PWD/elidinglabel.cpp \
    $$PWD/hostosinfo.cpp \
    $$PWD/tooltip/tooltip.cpp \
    $$PWD/tooltip/tips.cpp \
    $$PWD/unixutils.cpp \
    $$PWD/ansiescapecodehandler.cpp \
    $$PWD/execmenu.cpp \
    $$PWD/completinglineedit.cpp \
    $$PWD/winutils.cpp \
    $$PWD/itemviews.cpp \
    $$PWD/treemodel.cpp \
    $$PWD/treeviewcombobox.cpp \
    $$PWD/proxycredentialsdialog.cpp \
    $$PWD/macroexpander.cpp \
    $$PWD/theme/theme.cpp \
    $$PWD/progressindicator.cpp \
    $$PWD/fadingindicator.cpp \
    $$PWD/overridecursor.cpp \
    $$PWD/categorysortfiltermodel.cpp \
    $$PWD/dropsupport.cpp \
    $$PWD/icon.cpp \
    $$PWD/port.cpp \
    $$PWD/runextensions.cpp \
    $$PWD/utilsicons.cpp \
    $$PWD/guard.cpp

win32:SOURCES += $$PWD/consoleprocess_win.cpp
else:SOURCES += $$PWD/consoleprocess_unix.cpp

HEADERS += \
    $$PWD/environment.h \
    $$PWD/environmentmodel.h \
    $$PWD/environmentdialog.h \
    $$PWD/qtcprocess.h \
    $$PWD/utils_global.h \
    $$PWD/reloadpromptutils.h \
    $$PWD/settingsselector.h \
    $$PWD/shellcommand.h \
    $$PWD/shellcommandpage.h \
    $$PWD/stringutils.h \
    $$PWD/templateengine.h \
    $$PWD/temporarydirectory.h \
    $$PWD/temporaryfile.h \
    $$PWD/textfieldcheckbox.h \
    $$PWD/textfieldcombobox.h \
    $$PWD/filesearch.h \
    $$PWD/listutils.h \
    $$PWD/pathchooser.h \
    $$PWD/pathlisteditor.h \
    $$PWD/wizard.h \
    $$PWD/wizardpage.h \
    $$PWD/filewizardpage.h \
    $$PWD/filesystemwatcher.h \
    $$PWD/projectintropage.h \
    $$PWD/filenamevalidatinglineedit.h \
    $$PWD/codegeneration.h \
    $$PWD/newclasswidget.h \
    $$PWD/classnamevalidatinglineedit.h \
    $$PWD/linecolumnlabel.h \
    $$PWD/fancylineedit.h \
    $$PWD/qtcolorbutton.h \
    $$PWD/savedaction.h \
    $$PWD/consoleprocess.h \
    $$PWD/consoleprocess_p.h \
    $$PWD/synchronousprocess.h \
    $$PWD/savefile.h \
    $$PWD/fileutils.h \
    $$PWD/textfileformat.h \
    $$PWD/uncommentselection.h \
    $$PWD/parameteraction.h \
    $$PWD/headerviewstretcher.h \
    $$PWD/checkablemessagebox.h \
    $$PWD/qtcassert.h \
    $$PWD/styledbar.h \
    $$PWD/stylehelper.h \
    $$PWD/fancymainwindow.h \
    $$PWD/detailsbutton.h \
    $$PWD/detailswidget.h \
    $$PWD/changeset.h \
    $$PWD/faketooltip.h \
    $$PWD/htmldocextractor.h \
    $$PWD/navigationtreeview.h \
    $$PWD/crumblepath.h \
    $$PWD/historycompleter.h \
    $$PWD/buildablehelperlibrary.h \
    $$PWD/annotateditemdelegate.h \
    $$PWD/fileinprojectfinder.h \
    $$PWD/statuslabel.h \
    $$PWD/outputformatter.h \
    $$PWD/outputformat.h \
    $$PWD/flowlayout.h \
    $$PWD/networkaccessmanager.h \
    $$PWD/persistentsettings.h \
    $$PWD/completingtextedit.h \
    $$PWD/json.h \
    $$PWD/runextensions.h \
    $$PWD/portlist.h \
    $$PWD/processhandle.h \
    $$PWD/appmainwindow.h \
    $$PWD/basetreeview.h \
    $$PWD/elfreader.h \
    $$PWD/bracematcher.h \
    $$PWD/proxyaction.h \
    $$PWD/hostosinfo.h \
    $$PWD/osspecificaspects.h \
    $$PWD/elidinglabel.h \
    $$PWD/tooltip/tooltip.h \
    $$PWD/tooltip/tips.h \
    $$PWD/tooltip/reuse.h \
    $$PWD/tooltip/effects.h \
    $$PWD/unixutils.h \
    $$PWD/ansiescapecodehandler.h \
    $$PWD/execmenu.h \
    $$PWD/completinglineedit.h \
    $$PWD/winutils.h \
    $$PWD/itemviews.h \
    $$PWD/treemodel.h \
    $$PWD/treeviewcombobox.h \
    $$PWD/scopedswap.h \
    $$PWD/algorithm.h \
    $$PWD/QtConcurrentTools \
    $$PWD/proxycredentialsdialog.h \
    $$PWD/macroexpander.h \
    $$PWD/theme/theme.h \
    $$PWD/theme/theme_p.h \
    $$PWD/progressindicator.h \
    $$PWD/fadingindicator.h \
    $$PWD/executeondestruction.h \
    $$PWD/overridecursor.h \
    $$PWD/categorysortfiltermodel.h \
    $$PWD/dropsupport.h \
    $$PWD/utilsicons.h \
    $$PWD/icon.h \
    $$PWD/port.h \
    $$PWD/functiontraits.h \
    $$PWD/mapreduce.h \
    $$PWD/declarationmacros.h \
    $$PWD/smallstring.h \
    $$PWD/smallstringiterator.h \
    $$PWD/smallstringliteral.h \
    $$PWD/smallstringmemory.h \
    $$PWD/smallstringvector.h \
    $$PWD/smallstringlayout.h \
    $$PWD/sizedarray.h \
    $$PWD/smallstringio.h \
    $$PWD/guard.h \
    $$PWD/asconst.h \
    $$PWD/smallstringfwd.h

FORMS += $$PWD/filewizardpage.ui \
    $$PWD/projectintropage.ui \
    $$PWD/newclasswidget.ui \
    $$PWD/proxycredentialsdialog.ui

RESOURCES += $$PWD/utils.qrc

osx {
    HEADERS += \
        $$PWD/fileutils_mac.h
    OBJECTIVE_SOURCES += \
        $$PWD/fileutils_mac.mm
    LIBS += -framework Foundation
}

include(mimetypes/mimetypes.pri)
