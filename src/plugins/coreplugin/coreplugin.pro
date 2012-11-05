TEMPLATE = lib
TARGET = Core
DEFINES += CORE_LIBRARY
QT += network \
    script \
    sql

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += help printsupport
} else {
    CONFIG += help
}

include(../../qtcreatorplugin.pri)
include(../../libs/utils/utils.pri)
include(../../shared/scriptwrapper/scriptwrapper.pri)
include(coreplugin_dependencies.pri)
win32-msvc*:QMAKE_CXXFLAGS += -wd4251 -wd4290 -wd4250
INCLUDEPATH += dialogs \
    actionmanager \
    editormanager \
    progressmanager \
    scriptmanager
SOURCES += mainwindow.cpp \
    editmode.cpp \
    tabpositionindicator.cpp \
    fancyactionbar.cpp \
    fancytabwidget.cpp \
    generalsettings.cpp \
    id.cpp \
    icontext.cpp \
    messagemanager.cpp \
    messageoutputwindow.cpp \
    outputpane.cpp \
    outputwindow.cpp \
    vcsmanager.cpp \
    statusbarmanager.cpp \
    versiondialog.cpp \
    editormanager/editormanager.cpp \
    editormanager/editorview.cpp \
    editormanager/openeditorsmodel.cpp \
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
    dialogs/openwithdialog.cpp \
    progressmanager/progressmanager.cpp \
    progressmanager/progressview.cpp \
    progressmanager/progressbar.cpp \
    progressmanager/futureprogress.cpp \
    scriptmanager/scriptmanager.cpp \
    statusbarwidget.cpp \
    coreplugin.cpp \
    variablemanager.cpp \
    modemanager.cpp \
    basefilewizard.cpp \
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
    mimedatabase.cpp \
    icore.cpp \
    infobar.cpp \
    editormanager/ieditor.cpp \
    dialogs/ioptionspage.cpp \
    dialogs/iwizard.cpp \
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
    textdocument.cpp \
    documentmanager.cpp \
    removefiledialog.cpp \
    iversioncontrol.cpp

HEADERS += mainwindow.h \
    editmode.h \
    tabpositionindicator.h \
    fancyactionbar.h \
    fancytabwidget.h \
    generalsettings.h \
    id.h \
    messagemanager.h \
    messageoutputwindow.h \
    outputpane.h \
    outputwindow.h \
    vcsmanager.h \
    statusbarmanager.h \
    editormanager/editormanager.h \
    editormanager/editorview.h \
    editormanager/openeditorsmodel.h \
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
    dialogs/shortcutsettings.h \
    dialogs/openwithdialog.h \
    dialogs/iwizard.h \
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
    scriptmanager/metatypedeclarations.h \
    scriptmanager/scriptmanager.h \
    scriptmanager/scriptmanager_p.h \
    core_global.h \
    statusbarwidget.h \
    coreplugin.h \
    variablemanager.h \
    modemanager.h \
    basefilewizard.h \
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
    mimedatabase.h \
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
    removefiledialog.h

FORMS += dialogs/newdialog.ui \
    actionmanager/commandmappings.ui \
    dialogs/saveitemsdialog.ui \
    dialogs/openwithdialog.ui \
    generalsettings.ui \
    dialogs/externaltoolconfig.ui \
    variablechooser.ui \
    mimetypesettingspage.ui \
    mimetypemagicdialog.ui \
    removefiledialog.ui

RESOURCES += core.qrc \
    fancyactionbar.qrc

win32 {
    SOURCES += progressmanager/progressmanager_win.cpp
    greaterThan(QT_MAJOR_VERSION, 4): QT += gui-private # Uses QPlatformNativeInterface.
    LIBS += -lole32 -luser32
}
else:macx {
    HEADERS += macfullscreen.h
    OBJECTIVE_SOURCES += \
        progressmanager/progressmanager_mac.mm \
        macfullscreen.mm
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
OTHER_FILES += editormanager/BinFiles.mimetypes.xml
