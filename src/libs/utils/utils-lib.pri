dll {
    DEFINES += QTCREATOR_UTILS_LIB
} else {
    DEFINES += QTCREATOR_UTILS_STATIC_LIB
}

INCLUDEPATH += $$PWD
QT += network

SOURCES += $$PWD/environment.cpp \
    $$PWD/qtcprocess.cpp \
    $$PWD/reloadpromptutils.cpp \
    $$PWD/stringutils.cpp \
    $$PWD/filesearch.cpp \
    $$PWD/pathchooser.cpp \
    $$PWD/pathlisteditor.cpp \
    $$PWD/wizard.cpp \
    $$PWD/filewizardpage.cpp \
    $$PWD/filewizarddialog.cpp \
    $$PWD/projectintropage.cpp \
    $$PWD/basevalidatinglineedit.cpp \
    $$PWD/filenamevalidatinglineedit.cpp \
    $$PWD/projectnamevalidatinglineedit.cpp \
    $$PWD/codegeneration.cpp \
    $$PWD/newclasswidget.cpp \
    $$PWD/classnamevalidatinglineedit.cpp \
    $$PWD/linecolumnlabel.cpp \
    $$PWD/fancylineedit.cpp \
    $$PWD/qtcolorbutton.cpp \
    $$PWD/savedaction.cpp \
    $$PWD/submiteditorwidget.cpp \
    $$PWD/synchronousprocess.cpp \
    $$PWD/submitfieldwidget.cpp \
    $$PWD/consoleprocess.cpp \
    $$PWD/uncommentselection.cpp \
    $$PWD/parameteraction.cpp \
    $$PWD/treewidgetcolumnstretcher.cpp \
    $$PWD/checkablemessagebox.cpp \
    $$PWD/styledbar.cpp \
    $$PWD/stylehelper.cpp \
    $$PWD/welcomemodetreewidget.cpp \
    $$PWD/iwelcomepage.cpp \
    $$PWD/fancymainwindow.cpp \
    $$PWD/detailsbutton.cpp \
    $$PWD/detailswidget.cpp \
    $$PWD/changeset.cpp \
    $$PWD/filterlineedit.cpp \
    $$PWD/faketooltip.cpp \
    $$PWD/htmldocextractor.cpp \
    $$PWD/navigationtreeview.cpp \
    $$PWD/crumblepath.cpp \
    $$PWD/debuggerlanguagechooser.cpp \
    $$PWD/historycompleter.cpp \
    $$PWD/buildablehelperlibrary.cpp \
    $$PWD/annotateditemdelegate.cpp \
    $$PWD/fileinprojectfinder.cpp

win32 {
    SOURCES += $$PWD/abstractprocess_win.cpp \
        $$PWD/consoleprocess_win.cpp \
        $$PWD/winutils.cpp
    HEADERS += $$PWD/winutils.h
}
else:SOURCES += $$PWD/consoleprocess_unix.cpp

unix:!macx {
    HEADERS += $$PWD/unixutils.h
    SOURCES += $$PWD/unixutils.cpp
}
HEADERS += $$PWD/environment.h \
    $$PWD/qtcprocess.h \
    $$PWD/utils_global.h \
    $$PWD/reloadpromptutils.h \
    $$PWD/stringutils.h \
    $$PWD/filesearch.h \
    $$PWD/listutils.h \
    $$PWD/pathchooser.h \
    $$PWD/pathlisteditor.h \
    $$PWD/wizard.h \
    $$PWD/filewizardpage.h \
    $$PWD/filewizarddialog.h \
    $$PWD/projectintropage.h \
    $$PWD/basevalidatinglineedit.h \
    $$PWD/filenamevalidatinglineedit.h \
    $$PWD/projectnamevalidatinglineedit.h \
    $$PWD/codegeneration.h \
    $$PWD/newclasswidget.h \
    $$PWD/classnamevalidatinglineedit.h \
    $$PWD/linecolumnlabel.h \
    $$PWD/fancylineedit.h \
    $$PWD/qtcolorbutton.h \
    $$PWD/savedaction.h \
    $$PWD/submiteditorwidget.h \
    $$PWD/abstractprocess.h \
    $$PWD/consoleprocess.h \
    $$PWD/synchronousprocess.h \
    $$PWD/submitfieldwidget.h \
    $$PWD/uncommentselection.h \
    $$PWD/parameteraction.h \
    $$PWD/treewidgetcolumnstretcher.h \
    $$PWD/checkablemessagebox.h \
    $$PWD/qtcassert.h \
    $$PWD/styledbar.h \
    $$PWD/stylehelper.h \
    $$PWD/welcomemodetreewidget.h \
    $$PWD/iwelcomepage.h \
    $$PWD/fancymainwindow.h \
    $$PWD/detailsbutton.h \
    $$PWD/detailswidget.h \
    $$PWD/changeset.h \
    $$PWD/filterlineedit.h \
    $$PWD/faketooltip.h \
    $$PWD/htmldocextractor.h \
    $$PWD/navigationtreeview.h \
    $$PWD/crumblepath.h \
    $$PWD/debuggerlanguagechooser.h \
    $$PWD/historycompleter.h \
    $$PWD/buildablehelperlibrary.h \
    $$PWD/annotateditemdelegate.h \
    $$PWD/fileinprojectfinder.h

FORMS += $$PWD/filewizardpage.ui \
    $$PWD/projectintropage.ui \
    $$PWD/newclasswidget.ui \
    $$PWD/submiteditorwidget.ui \
    $$PWD/checkablemessagebox.ui
RESOURCES += $$PWD/utils.qrc
