DEFINES += CORE_LIBRARY
QT += help \
    network \
    printsupport \
    qml \
    sql

# embedding build time information prevents repeatedly binary exact versions from same source code
isEmpty(QTC_SHOW_BUILD_DATE): QTC_SHOW_BUILD_DATE = $$(QTC_SHOW_BUILD_DATE)
!isEmpty(QTC_SHOW_BUILD_DATE): DEFINES += QTC_SHOW_BUILD_DATE

include(../../qtcreatorplugin.pri)
win32-msvc*:QMAKE_CXXFLAGS += -wd4251 -wd4290 -wd4250
SOURCES += corejsextensions.cpp \
    mainwindow.cpp \
    editmode.cpp \
    iwizardfactory.cpp \
    tabpositionindicator.cpp \
    fancyactionbar.cpp \
    fancytabwidget.cpp \
    generalsettings.cpp \
    themesettings.cpp \
    themesettingswidget.cpp \
    id.cpp \
    icontext.cpp \
    jsexpander.cpp \
    messagemanager.cpp \
    messageoutputwindow.cpp \
    outputpane.cpp \
    outputwindow.cpp \
    vcsmanager.cpp \
    statusbarmanager.cpp \
    versiondialog.cpp \
    editormanager/editorarea.cpp \
    editormanager/editormanager.cpp \
    editormanager/editorview.cpp \
    editormanager/editorwindow.cpp \
    editormanager/documentmodel.cpp \
    editormanager/openeditorsview.cpp \
    editormanager/openeditorswindow.cpp \
    editormanager/ieditorfactory.cpp \
    editormanager/iexternaleditor.cpp \
    actionmanager/actionmanager.cpp \
    actionmanager/command.cpp \
    actionmanager/commandbutton.cpp \
    actionmanager/actioncontainer.cpp \
    actionmanager/commandsfile.cpp \
    dialogs/saveitemsdialog.cpp \
    dialogs/newdialog.cpp \
    dialogs/settingsdialog.cpp \
    actionmanager/commandmappings.cpp \
    dialogs/shortcutsettings.cpp \
    dialogs/readonlyfilesdialog.cpp \
    dialogs/openwithdialog.cpp \
    progressmanager/progressmanager.cpp \
    progressmanager/progressview.cpp \
    progressmanager/progressbar.cpp \
    progressmanager/futureprogress.cpp \
    statusbarwidget.cpp \
    coreplugin.cpp \
    modemanager.cpp \
    basefilewizard.cpp \
    basefilewizardfactory.cpp \
    generatedfile.cpp \
    plugindialog.cpp \
    inavigationwidgetfactory.cpp \
    navigationwidget.cpp \
    manhattanstyle.cpp \
    minisplitter.cpp \
    styleanimator.cpp \
    findplaceholder.cpp \
    rightpane.cpp \
    sidebar.cpp \
    fileiconprovider.cpp \
    icore.cpp \
    infobar.cpp \
    editormanager/ieditor.cpp \
    dialogs/ioptionspage.cpp \
    settingsdatabase.cpp \
    imode.cpp \
    editormanager/systemeditor.cpp \
    designmode.cpp \
    editortoolbar.cpp \
    helpmanager.cpp \
    outputpanemanager.cpp \
    navigationsubwidget.cpp \
    sidebarwidget.cpp \
    externaltool.cpp \
    dialogs/externaltoolconfig.cpp \
    toolsettings.cpp \
    variablechooser.cpp \
    mimetypemagicdialog.cpp \
    mimetypesettings.cpp \
    dialogs/promptoverwritedialog.cpp \
    fileutils.cpp \
    featureprovider.cpp \
    idocument.cpp \
    idocumentfactory.cpp \
    textdocument.cpp \
    documentmanager.cpp \
    removefiledialog.cpp \
    iversioncontrol.cpp \
    dialogs/addtovcsdialog.cpp \
    icorelistener.cpp \
    ioutputpane.cpp \
    patchtool.cpp \
    windowsupport.cpp \
    opendocumentstreeview.cpp \
    themeeditor/themecolors.cpp \
    themeeditor/themecolorstableview.cpp \
    themeeditor/colorvariable.cpp \
    themeeditor/themeeditorwidget.cpp \
    themeeditor/colorrole.cpp \
    themeeditor/themesettingstablemodel.cpp \
    themeeditor/sectionedtablemodel.cpp \
    themeeditor/themesettingsitemdelegate.cpp \
    messagebox.cpp \
    iwelcomepage.cpp \
    externaltoolmanager.cpp

HEADERS += corejsextensions.h \
    mainwindow.h \
    editmode.h \
    iwizardfactory.h \
    tabpositionindicator.h \
    fancyactionbar.h \
    fancytabwidget.h \
    generalsettings.h \
    themesettings.h \
    themesettingswidget.h \
    id.h \
    jsexpander.h \
    messagemanager.h \
    messageoutputwindow.h \
    outputpane.h \
    outputwindow.h \
    vcsmanager.h \
    statusbarmanager.h \
    editormanager/editorarea.h \
    editormanager/editormanager.h \
    editormanager/editormanager_p.h \
    editormanager/editorview.h \
    editormanager/editorwindow.h \
    editormanager/documentmodel.h \
    editormanager/openeditorsview.h \
    editormanager/openeditorswindow.h \
    editormanager/ieditor.h \
    editormanager/iexternaleditor.h \
    editormanager/ieditorfactory.h \
    actionmanager/actioncontainer.h \
    actionmanager/actionmanager.h \
    actionmanager/command.h \
    actionmanager/commandbutton.h \
    actionmanager/actionmanager_p.h \
    actionmanager/command_p.h \
    actionmanager/actioncontainer_p.h \
    actionmanager/commandsfile.h \
    dialogs/saveitemsdialog.h \
    dialogs/newdialog.h \
    dialogs/settingsdialog.h \
    actionmanager/commandmappings.h \
    dialogs/readonlyfilesdialog.h \
    dialogs/shortcutsettings.h \
    dialogs/openwithdialog.h \
    dialogs/ioptionspage.h \
    progressmanager/progressmanager_p.h \
    progressmanager/progressview.h \
    progressmanager/progressbar.h \
    progressmanager/futureprogress.h \
    progressmanager/progressmanager.h \
    icontext.h \
    icore.h \
    infobar.h \
    imode.h \
    ioutputpane.h \
    coreconstants.h \
    iversioncontrol.h \
    ifilewizardextension.h \
    icorelistener.h \
    versiondialog.h \
    core_global.h \
    statusbarwidget.h \
    coreplugin.h \
    modemanager.h \
    basefilewizard.h \
    basefilewizardfactory.h \
    generatedfile.h \
    plugindialog.h \
    inavigationwidgetfactory.h \
    navigationwidget.h \
    manhattanstyle.h \
    minisplitter.h \
    styleanimator.h \
    findplaceholder.h \
    rightpane.h \
    sidebar.h \
    fileiconprovider.h \
    settingsdatabase.h \
    editormanager/systemeditor.h \
    designmode.h \
    editortoolbar.h \
    helpmanager.h \
    outputpanemanager.h \
    navigationsubwidget.h \
    sidebarwidget.h \
    externaltool.h \
    dialogs/externaltoolconfig.h \
    toolsettings.h \
    variablechooser.h \
    mimetypemagicdialog.h \
    mimetypesettings.h \
    dialogs/promptoverwritedialog.h \
    fileutils.h \
    externaltoolmanager.h \
    generatedfile.h \
    featureprovider.h \
    idocument.h \
    idocumentfactory.h \
    textdocument.h \
    documentmanager.h \
    removefiledialog.h \
    dialogs/addtovcsdialog.h \
    patchtool.h \
    windowsupport.h \
    opendocumentstreeview.h \
    themeeditor/themecolors.h \
    themeeditor/themecolorstableview.h \
    themeeditor/colorvariable.h \
    themeeditor/themeeditorwidget.h \
    themeeditor/colorrole.h \
    themeeditor/themesettingstablemodel.h \
    themeeditor/sectionedtablemodel.h \
    themeeditor/themesettingsitemdelegate.h \
    messagebox.h \
    iwelcomepage.h

FORMS += dialogs/newdialog.ui \
    dialogs/saveitemsdialog.ui \
    dialogs/readonlyfilesdialog.ui \
    dialogs/openwithdialog.ui \
    generalsettings.ui \
    themesettings.ui \
    dialogs/externaltoolconfig.ui \
    mimetypesettingspage.ui \
    mimetypemagicdialog.ui \
    removefiledialog.ui \
    dialogs/addtovcsdialog.ui \
    themeeditor/themeeditorwidget.ui

RESOURCES += core.qrc \
    fancyactionbar.qrc

include(find/find.pri)
include(locator/locator.pri)

win32 {
    SOURCES += progressmanager/progressmanager_win.cpp
    QT += gui-private # Uses QPlatformNativeInterface.
    LIBS += -lole32 -luser32
}
else:macx {
    OBJECTIVE_SOURCES += \
        progressmanager/progressmanager_mac.mm
    LIBS += -framework AppKit
}
else:unix {
    SOURCES += progressmanager/progressmanager_x11.cpp

    IMAGE_SIZE_LIST = 16 24 32 48 64 128 256 512

    for(imagesize, IMAGE_SIZE_LIST) {
        eval(image$${imagesize}.files = images/logo/$${imagesize}/QtProject-qtcreator.png)
        eval(image$${imagesize}.path = $$QTC_PREFIX/share/icons/hicolor/$${imagesize}x$${imagesize}/apps)
        INSTALLS += image$${imagesize}
    }
}
DISTFILES += editormanager/BinFiles.mimetypes.xml

equals(TEST, 1) {
    SOURCES += testdatadir.cpp
    HEADERS += testdatadir.h
}
