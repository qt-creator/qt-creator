TEMPLATE = lib
TARGET = Core
DEFINES += CORE_LIBRARY
QT += xml \
    script \
    svg

include(../../qworkbenchplugin.pri)
include(../../libs/utils/utils.pri)
include(../../shared/scriptwrapper/scriptwrapper.pri)
include(coreplugin_dependencies.pri)
INCLUDEPATH += dialogs \
    actionmanager \
    editormanager \
    progressmanager \
    scriptmanager
DEPENDPATH += dialogs \
    actionmanager \
    editormanager \
    scriptmanager
SOURCES += mainwindow.cpp \
    welcomemode.cpp \
    editmode.cpp \
    tabpositionindicator.cpp \
    fancyactionbar.cpp \
    fancytabwidget.cpp \
    flowlayout.cpp \
    generalsettings.cpp \
    filemanager.cpp \
    uniqueidmanager.cpp \
    messagemanager.cpp \
    messageoutputwindow.cpp \
    outputpane.cpp \
    vcsmanager.cpp \
    viewmanager.cpp \
    versiondialog.cpp \
    editormanager/editorgroup.cpp \
    editormanager/editormanager.cpp \
    editormanager/stackededitorgroup.cpp \
    editormanager/editorsplitter.cpp \
    editormanager/openeditorsview.cpp \
    editormanager/openeditorswindow.cpp \
    actionmanager/actionmanager.cpp \
    actionmanager/command.cpp \
    actionmanager/actioncontainer.cpp \
    actionmanager/commandsfile.cpp \
    dialogs/saveitemsdialog.cpp \
    dialogs/newdialog.cpp \
    dialogs/settingsdialog.cpp \
    dialogs/shortcutsettings.cpp \
    dialogs/openwithdialog.cpp \
    progressmanager/progressmanager.cpp \
    progressmanager/progressview.cpp \
    progressmanager/progresspie.cpp \
    progressmanager/futureprogress.cpp \
    scriptmanager/scriptmanager.cpp \
    scriptmanager/qworkbench_wrapper.cpp \
    basemode.cpp \
    baseview.cpp \
    coreplugin.cpp \
    variablemanager.cpp \
    modemanager.cpp \
    coreimpl.cpp \
    basefilewizard.cpp \
    plugindialog.cpp \
    stylehelper.cpp \
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
    icore.cpp
HEADERS += mainwindow.h \
    welcomemode.h \
    editmode.h \
    tabpositionindicator.h \
    fancyactionbar.h \
    fancytabwidget.h \
    flowlayout.h \
    generalsettings.h \
    filemanager.h \
    uniqueidmanager.h \
    messagemanager.h \
    messageoutputwindow.h \
    outputpane.h \
    vcsmanager.h \
    viewmanager.h \
    editormanager/editorgroup.h \
    editormanager/editormanager.h \
    editormanager/stackededitorgroup.h \
    editormanager/editorsplitter.h \
    editormanager/openeditorsview.h \
    editormanager/openeditorswindow.h \
    editormanager/ieditor.h \
    editormanager/ieditorfactory.h \
    actionmanager/actioncontainer.h \
    actionmanager/actionmanager.h \
    actionmanager/command.h \
    actionmanager/actionmanager_p.h \
    actionmanager/command_p.h \
    actionmanager/actioncontainer_p.h \
    actionmanager/commandsfile.h \
    dialogs/saveitemsdialog.h \
    dialogs/newdialog.h \
    dialogs/settingsdialog.h \
    dialogs/shortcutsettings.h \
    dialogs/openwithdialog.h \
    dialogs/iwizard.h \
    dialogs/ioptionspage.h \
    progressmanager/progressmanager_p.h \
    progressmanager/progressview.h \
    progressmanager/progresspie.h \
    progressmanager/futureprogress.h \
    progressmanager/progressmanager.h \
    icontext.h \
    icore.h \
    ifile.h \
    ifilefactory.h \
    imode.h \
    ioutputpane.h \
    coreconstants.h \
    iversioncontrol.h \
    iview.h \
    ifilewizardextension.h \
    viewmanagerinterface.h \
    icorelistener.h \
    versiondialog.h \
    scriptmanager/metatypedeclarations.h \
    scriptmanager/qworkbench_wrapper.h \
    scriptmanager/scriptmanager.h \
    scriptmanager/scriptmanager_p.h \
    core_global.h \
    basemode.h \
    baseview.h \
    coreplugin.h \
    variablemanager.h \
    modemanager.h \
    coreimpl.h \
    basefilewizard.h \
    plugindialog.h \
    stylehelper.h \
    inavigationwidgetfactory.h \
    navigationwidget.h \
    manhattanstyle.h \
    minisplitter.h \
    styleanimator.h \
    findplaceholder.h \
    rightpane.h \
    sidebar.h \
    fileiconprovider.h \
    mimedatabase.h
FORMS += dialogs/newdialog.ui \
    dialogs/settingsdialog.ui \
    dialogs/shortcutsettings.ui \
    dialogs/saveitemsdialog.ui \
    dialogs/openwithdialog.ui \
    editormanager/openeditorsview.ui \
    generalsettings.ui
RESOURCES += core.qrc \
    fancyactionbar.qrc

contains(QT_CONFIG, webkit) {
    QT += webkit
}

linux-* {
    images.files = images/qtcreator_logo_*.png
    images.path  = /share/pixmaps

    INSTALLS += images
}
