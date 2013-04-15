/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "mainwindow.h"
#include "actioncontainer.h"
#include "command.h"
#include "actionmanager.h"
#include "actionmanager_p.h"
#include "icore.h"
#include "coreconstants.h"
#include "editormanager.h"
#include "toolsettings.h"
#include "mimetypesettings.h"
#include "fancytabwidget.h"
#include "documentmanager.h"
#include "generalsettings.h"
#include "helpmanager.h"
#include "ieditor.h"
#include "idocumentfactory.h"
#include "messagemanager.h"
#include "modemanager.h"
#include "mimedatabase.h"
#include "newdialog.h"
#include "outputpanemanager.h"
#include "outputpane.h"
#include "plugindialog.h"
#include "progressmanager_p.h"
#include "progressview.h"
#include "shortcutsettings.h"
#include "vcsmanager.h"
#include "scriptmanager_p.h"
#include "settingsdialog.h"
#include "variablemanager.h"
#include "versiondialog.h"
#include "statusbarmanager.h"
#include "id.h"
#include "manhattanstyle.h"
#include "navigationwidget.h"
#include "rightpane.h"
#include "editormanager/ieditorfactory.h"
#include "statusbarwidget.h"
#include "externaltoolmanager.h"
#include "editormanager/systemeditor.h"

#if defined(Q_OS_MAC)
#include "macfullscreen.h"
#endif

#include <app/app_version.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icorelistener.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <coreplugin/settingsdatabase.h>
#include <utils/historycompleter.h>
#include <utils/hostosinfo.h>
#include <utils/stylehelper.h>
#include <utils/stringutils.h>
#include <extensionsystem/pluginmanager.h>

#include <QDebug>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QDir>
#include <QMimeData>

#include <QApplication>
#include <QCloseEvent>
#include <QMenu>
#include <QPrinter>
#include <QShortcut>
#include <QStatusBar>
#include <QToolButton>
#include <QMessageBox>
#include <QMenuBar>
#include <QPushButton>
#include <QStyleFactory>

/*
#include <signal.h>
extern "C" void handleSigInt(int sig)
{
    Q_UNUSED(sig)
    Core::ICore::exit();
    qDebug() << "SIGINT caught. Shutting down.";
}
*/

using namespace Core;
using namespace Core::Internal;

enum { debugMainWindow = 0 };

MainWindow::MainWindow() :
    Utils::AppMainWindow(),
    m_coreImpl(new ICore(this)),
    m_additionalContexts(Constants::C_GLOBAL),
    m_settings(ExtensionSystem::PluginManager::settings()),
    m_globalSettings(ExtensionSystem::PluginManager::globalSettings()),
    m_settingsDatabase(new SettingsDatabase(QFileInfo(m_settings->fileName()).path(),
                                            QLatin1String("QtCreator"),
                                            this)),
    m_printer(0),
    m_actionManager(new ActionManager(this)),
    m_editorManager(0),
    m_externalToolManager(0),
    m_progressManager(new ProgressManagerPrivate()),
    m_scriptManager(new ScriptManagerPrivate(this)),
    m_variableManager(new VariableManager),
    m_vcsManager(new VcsManager),
    m_statusBarManager(0),
    m_modeManager(0),
    m_mimeDatabase(new MimeDatabase),
    m_helpManager(new HelpManager),
    m_navigationWidget(0),
    m_rightPaneWidget(0),
    m_versionDialog(0),
    m_activeContext(0),
    m_generalSettings(new GeneralSettings),
    m_shortcutSettings(new ShortcutSettings),
    m_toolSettings(new ToolSettings),
    m_mimeTypeSettings(new MimeTypeSettings),
    m_systemEditor(new SystemEditor),
    m_focusToEditor(0),
    m_newAction(0),
    m_openAction(0),
    m_openWithAction(0),
    m_saveAllAction(0),
    m_exitAction(0),
    m_optionsAction(0),
    m_toggleSideBarAction(0),
    m_toggleFullScreenAction(0),
    m_minimizeAction(0),
    m_zoomAction(0),
    m_toggleSideBarButton(new QToolButton)
{
    (void) new DocumentManager(this);
    OutputPaneManager::create();

    Utils::HistoryCompleter::setSettings(m_settings);

    setWindowTitle(tr("Qt Creator"));
    if (!Utils::HostOsInfo::isMacHost())
        QApplication::setWindowIcon(QIcon(QLatin1String(Constants::ICON_QTLOGO_128)));
    QCoreApplication::setApplicationName(QLatin1String("QtCreator"));
    QCoreApplication::setApplicationVersion(QLatin1String(Core::Constants::IDE_VERSION_LONG));
    QCoreApplication::setOrganizationName(QLatin1String(Constants::IDE_SETTINGSVARIANT_STR));
    QString baseName = QApplication::style()->objectName();
    if (Utils::HostOsInfo::isAnyUnixHost() && !Utils::HostOsInfo::isMacHost()) {
        if (baseName == QLatin1String("windows")) {
            // Sometimes we get the standard windows 95 style as a fallback
            if (QStyleFactory::keys().contains(QLatin1String("Fusion")))
                baseName = QLatin1String("fusion"); // Qt5
            else { // Qt4
                // e.g. if we are running on a KDE4 desktop
                QByteArray desktopEnvironment = qgetenv("DESKTOP_SESSION");
                if (desktopEnvironment == "kde")
                    baseName = QLatin1String("plastique");
                else
                    baseName = QLatin1String("cleanlooks");
            }
        }
    }
    qApp->setStyle(new ManhattanStyle(baseName));

    setDockNestingEnabled(true);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    registerDefaultContainers();
    registerDefaultActions();

    m_navigationWidget = new NavigationWidget(m_toggleSideBarAction);
    m_rightPaneWidget = new RightPaneWidget();

    m_modeStack = new FancyTabWidget(this);
    m_modeManager = new ModeManager(this, m_modeStack);
    m_statusBarManager = new StatusBarManager(this);
    m_messageManager = new MessageManager;
    m_editorManager = new EditorManager(this);
    m_editorManager->hide();
    m_externalToolManager = new ExternalToolManager();
    setCentralWidget(m_modeStack);

    m_progressManager->progressView()->setParent(this);
    m_progressManager->progressView()->setReferenceWidget(m_modeStack->statusBar());

    connect(QApplication::instance(), SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(updateFocusWidget(QWidget*,QWidget*)));
    // Add a small Toolbutton for toggling the navigation widget
    statusBar()->insertPermanentWidget(0, m_toggleSideBarButton);

//    setUnifiedTitleAndToolBarOnMac(true);
    //if (Utils::HostOsInfo::isAnyUnixHost())
        //signal(SIGINT, handleSigInt);

    statusBar()->setProperty("p_styled", true);
    setAcceptDrops(true);

#if defined(Q_OS_MAC)
    MacFullScreen::addFullScreen(this);
#endif
}

void MainWindow::setSidebarVisible(bool visible)
{
    if (NavigationWidgetPlaceHolder::current()) {
        if (m_navigationWidget->isSuppressed() && visible) {
            m_navigationWidget->setShown(true);
            m_navigationWidget->setSuppressed(false);
        } else {
            m_navigationWidget->setShown(visible);
        }
    }
}

void MainWindow::setSuppressNavigationWidget(bool suppress)
{
    if (NavigationWidgetPlaceHolder::current())
        m_navigationWidget->setSuppressed(suppress);
}

void MainWindow::setOverrideColor(const QColor &color)
{
    m_overrideColor = color;
}

void MainWindow::setIsFullScreen(bool fullScreen)
{
    if (fullScreen)
        m_toggleFullScreenAction->setText(tr("Exit Full Screen"));
    else
        m_toggleFullScreenAction->setText(tr("Enter Full Screen"));
}

MainWindow::~MainWindow()
{
    ExtensionSystem::PluginManager::removeObject(m_shortcutSettings);
    ExtensionSystem::PluginManager::removeObject(m_generalSettings);
    ExtensionSystem::PluginManager::removeObject(m_toolSettings);
    ExtensionSystem::PluginManager::removeObject(m_mimeTypeSettings);
    ExtensionSystem::PluginManager::removeObject(m_systemEditor);
    delete m_externalToolManager;
    m_externalToolManager = 0;
    delete m_messageManager;
    m_messageManager = 0;
    delete m_shortcutSettings;
    m_shortcutSettings = 0;
    delete m_generalSettings;
    m_generalSettings = 0;
    delete m_toolSettings;
    m_toolSettings = 0;
    delete m_mimeTypeSettings;
    m_mimeTypeSettings = 0;
    delete m_systemEditor;
    m_systemEditor = 0;
    delete m_settings;
    m_settings = 0;
    delete m_printer;
    m_printer = 0;
    delete m_vcsManager;
    m_vcsManager = 0;
    //we need to delete editormanager and statusbarmanager explicitly before the end of the destructor,
    //because they might trigger stuff that tries to access data from editorwindow, like removeContextWidget

    // All modes are now gone
    OutputPaneManager::destroy();

    // Now that the OutputPaneManager is gone, is a good time to delete the view
    ExtensionSystem::PluginManager::removeObject(m_outputView);
    delete m_outputView;

    delete m_editorManager;
    m_editorManager = 0;
    delete m_progressManager;
    m_progressManager = 0;
    delete m_statusBarManager;
    m_statusBarManager = 0;
    ExtensionSystem::PluginManager::removeObject(m_coreImpl);
    delete m_coreImpl;
    m_coreImpl = 0;

    delete m_rightPaneWidget;
    m_rightPaneWidget = 0;

    delete m_modeManager;
    m_modeManager = 0;
    delete m_mimeDatabase;
    m_mimeDatabase = 0;

    delete m_helpManager;
    m_helpManager = 0;
}

bool MainWindow::init(QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    if (!mimeDatabase()->addMimeTypes(QLatin1String(":/core/editormanager/BinFiles.mimetypes.xml"), errorMessage))
        return false;

    ExtensionSystem::PluginManager::addObject(m_coreImpl);
    m_statusBarManager->init();
    m_modeManager->init();
    m_progressManager->init(); // needs the status bar manager

    ExtensionSystem::PluginManager::addObject(m_generalSettings);
    ExtensionSystem::PluginManager::addObject(m_shortcutSettings);
    ExtensionSystem::PluginManager::addObject(m_toolSettings);
    ExtensionSystem::PluginManager::addObject(m_mimeTypeSettings);
    ExtensionSystem::PluginManager::addObject(m_systemEditor);

    // Add widget to the bottom, we create the view here instead of inside the
    // OutputPaneManager, since the StatusBarManager needs to be initialized before
    m_outputView = new Core::StatusBarWidget;
    m_outputView->setWidget(OutputPaneManager::instance()->buttonsWidget());
    m_outputView->setPosition(Core::StatusBarWidget::Second);
    ExtensionSystem::PluginManager::addObject(m_outputView);
    m_messageManager->init();
    return true;
}

void MainWindow::extensionsInitialized()
{
    m_editorManager->init();
    m_statusBarManager->extensionsInitalized();
    OutputPaneManager::instance()->init();
    m_vcsManager->extensionsInitialized();
    m_navigationWidget->setFactories(ExtensionSystem::PluginManager::getObjects<INavigationWidgetFactory>());

    // reading the shortcut settings must be done after all shortcuts have been registered
    m_actionManager->d->initialize();

    readSettings();
    updateContext();

    emit m_coreImpl->coreAboutToOpen();
    show();
    emit m_coreImpl->coreOpened();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    ICore::saveSettings();

    // Save opened files
    bool cancelled;
    QList<IDocument*> notSaved = DocumentManager::saveModifiedDocuments(DocumentManager::modifiedDocuments(), &cancelled);
    if (cancelled || !notSaved.isEmpty()) {
        event->ignore();
        return;
    }

    const QList<ICoreListener *> listeners =
        ExtensionSystem::PluginManager::getObjects<ICoreListener>();
    foreach (ICoreListener *listener, listeners) {
        if (!listener->coreAboutToClose()) {
            event->ignore();
            return;
        }
    }

    emit m_coreImpl->coreAboutToClose();

    writeSettings();

    m_navigationWidget->closeSubWidgets();

    event->accept();
}

// Check for desktop file manager file drop events

static bool isDesktopFileManagerDrop(const QMimeData *d, QStringList *files = 0)
{
    if (files)
        files->clear();
    // Extract dropped files from Mime data.
    if (!d->hasUrls())
        return false;
    const QList<QUrl> urls = d->urls();
    if (urls.empty())
        return false;
    // Try to find local files
    bool hasFiles = false;
    const QList<QUrl>::const_iterator cend = urls.constEnd();
    for (QList<QUrl>::const_iterator it = urls.constBegin(); it != cend; ++it) {
        const QString fileName = it->toLocalFile();
        if (!fileName.isEmpty()) {
            hasFiles = true;
            if (files)
                files->push_back(fileName);
            else
                break; // No result list, sufficient for checking
        }
    }
    return hasFiles;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (isDesktopFileManagerDrop(event->mimeData()) && m_filesToOpenDelayed.isEmpty())
        event->accept();
    else
        event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QStringList files;
    if (isDesktopFileManagerDrop(event->mimeData(), &files)) {
        event->accept();
        m_filesToOpenDelayed.append(files);
        QTimer::singleShot(50, this, SLOT(openDelayedFiles()));
    } else {
        event->ignore();
    }
}

void MainWindow::openDelayedFiles()
{
    if (m_filesToOpenDelayed.isEmpty())
        return;
    raiseWindow();
    openFiles(m_filesToOpenDelayed, ICore::SwitchMode);
    m_filesToOpenDelayed.clear();
}

IContext *MainWindow::currentContextObject() const
{
    return m_activeContext;
}

QStatusBar *MainWindow::statusBar() const
{
    return m_modeStack->statusBar();
}

void MainWindow::registerDefaultContainers()
{
    ActionContainer *menubar = ActionManager::createMenuBar(Constants::MENU_BAR);

    if (!Utils::HostOsInfo::isMacHost()) // System menu bar on Mac
        setMenuBar(menubar->menuBar());
    menubar->appendGroup(Constants::G_FILE);
    menubar->appendGroup(Constants::G_EDIT);
    menubar->appendGroup(Constants::G_VIEW);
    menubar->appendGroup(Constants::G_TOOLS);
    menubar->appendGroup(Constants::G_WINDOW);
    menubar->appendGroup(Constants::G_HELP);

    // File Menu
    ActionContainer *filemenu = ActionManager::createMenu(Constants::M_FILE);
    menubar->addMenu(filemenu, Constants::G_FILE);
    filemenu->menu()->setTitle(tr("&File"));
    filemenu->appendGroup(Constants::G_FILE_NEW);
    filemenu->appendGroup(Constants::G_FILE_OPEN);
    filemenu->appendGroup(Constants::G_FILE_PROJECT);
    filemenu->appendGroup(Constants::G_FILE_SAVE);
    filemenu->appendGroup(Constants::G_FILE_CLOSE);
    filemenu->appendGroup(Constants::G_FILE_PRINT);
    filemenu->appendGroup(Constants::G_FILE_OTHER);
    connect(filemenu->menu(), SIGNAL(aboutToShow()), this, SLOT(aboutToShowRecentFiles()));


    // Edit Menu
    ActionContainer *medit = ActionManager::createMenu(Constants::M_EDIT);
    menubar->addMenu(medit, Constants::G_EDIT);
    medit->menu()->setTitle(tr("&Edit"));
    medit->appendGroup(Constants::G_EDIT_UNDOREDO);
    medit->appendGroup(Constants::G_EDIT_COPYPASTE);
    medit->appendGroup(Constants::G_EDIT_SELECTALL);
    medit->appendGroup(Constants::G_EDIT_ADVANCED);
    medit->appendGroup(Constants::G_EDIT_FIND);
    medit->appendGroup(Constants::G_EDIT_OTHER);

    // Tools Menu
    ActionContainer *ac = ActionManager::createMenu(Constants::M_TOOLS);
    menubar->addMenu(ac, Constants::G_TOOLS);
    ac->menu()->setTitle(tr("&Tools"));

    // Window Menu
    ActionContainer *mwindow = ActionManager::createMenu(Constants::M_WINDOW);
    menubar->addMenu(mwindow, Constants::G_WINDOW);
    mwindow->menu()->setTitle(tr("&Window"));
    mwindow->appendGroup(Constants::G_WINDOW_SIZE);
    mwindow->appendGroup(Constants::G_WINDOW_VIEWS);
    mwindow->appendGroup(Constants::G_WINDOW_PANES);
    mwindow->appendGroup(Constants::G_WINDOW_SPLIT);
    mwindow->appendGroup(Constants::G_WINDOW_NAVIGATE);
    mwindow->appendGroup(Constants::G_WINDOW_OTHER);

    // Help Menu
    ac = ActionManager::createMenu(Constants::M_HELP);
    menubar->addMenu(ac, Constants::G_HELP);
    ac->menu()->setTitle(tr("&Help"));
    ac->appendGroup(Constants::G_HELP_HELP);
    ac->appendGroup(Constants::G_HELP_ABOUT);
}

void MainWindow::registerDefaultActions()
{
    ActionContainer *mfile = ActionManager::actionContainer(Constants::M_FILE);
    ActionContainer *medit = ActionManager::actionContainer(Constants::M_EDIT);
    ActionContainer *mtools = ActionManager::actionContainer(Constants::M_TOOLS);
    ActionContainer *mwindow = ActionManager::actionContainer(Constants::M_WINDOW);
    ActionContainer *mhelp = ActionManager::actionContainer(Constants::M_HELP);

    Context globalContext(Constants::C_GLOBAL);

    // File menu separators
    mfile->addSeparator(globalContext, Constants::G_FILE_SAVE);
    mfile->addSeparator(globalContext, Constants::G_FILE_PRINT);
    mfile->addSeparator(globalContext, Constants::G_FILE_CLOSE);
    mfile->addSeparator(globalContext, Constants::G_FILE_OTHER);
    // Edit menu separators
    medit->addSeparator(globalContext, Constants::G_EDIT_COPYPASTE);
    medit->addSeparator(globalContext, Constants::G_EDIT_SELECTALL);
    medit->addSeparator(globalContext, Constants::G_EDIT_FIND);
    medit->addSeparator(globalContext, Constants::G_EDIT_ADVANCED);

    // Return to editor shortcut: Note this requires Qt to fix up
    // handling of shortcut overrides in menus, item views, combos....
    m_focusToEditor = new QShortcut(this);
    Command *cmd = ActionManager::registerShortcut(m_focusToEditor, Constants::S_RETURNTOEDITOR, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_Escape));
    connect(m_focusToEditor, SIGNAL(activated()), this, SLOT(setFocusToEditor()));

    // New File Action
    QIcon icon = QIcon::fromTheme(QLatin1String("document-new"), QIcon(QLatin1String(Constants::ICON_NEWFILE)));
    m_newAction = new QAction(icon, tr("&New File or Project..."), this);
    cmd = ActionManager::registerAction(m_newAction, Constants::NEW, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::New);
    mfile->addAction(cmd, Constants::G_FILE_NEW);
    connect(m_newAction, SIGNAL(triggered()), this, SLOT(newFile()));

    // Open Action
    icon = QIcon::fromTheme(QLatin1String("document-open"), QIcon(QLatin1String(Constants::ICON_OPENFILE)));
    m_openAction = new QAction(icon, tr("&Open File or Project..."), this);
    cmd = ActionManager::registerAction(m_openAction, Constants::OPEN, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Open);
    mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openAction, SIGNAL(triggered()), this, SLOT(openFile()));

    // Open With Action
    m_openWithAction = new QAction(tr("Open File &With..."), this);
    cmd = ActionManager::registerAction(m_openWithAction, Constants::OPEN_WITH, globalContext);
    mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openWithAction, SIGNAL(triggered()), this, SLOT(openFileWith()));

    // File->Recent Files Menu
    ActionContainer *ac = ActionManager::createMenu(Constants::M_FILE_RECENTFILES);
    mfile->addMenu(ac, Constants::G_FILE_OPEN);
    ac->menu()->setTitle(tr("Recent &Files"));
    ac->setOnAllDisabledBehavior(ActionContainer::Show);

    // Save Action
    icon = QIcon::fromTheme(QLatin1String("document-save"), QIcon(QLatin1String(Constants::ICON_SAVEFILE)));
    QAction *tmpaction = new QAction(icon, tr("&Save"), this);
    tmpaction->setEnabled(false);
    cmd = ActionManager::registerAction(tmpaction, Constants::SAVE, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Save);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Save"));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // Save As Action
    icon = QIcon::fromTheme(QLatin1String("document-save-as"));
    tmpaction = new QAction(icon, tr("Save &As..."), this);
    tmpaction->setEnabled(false);
    cmd = ActionManager::registerAction(tmpaction, Constants::SAVEAS, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Shift+S") : QString()));
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Save As..."));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // SaveAll Action
    m_saveAllAction = new QAction(tr("Save A&ll"), this);
    cmd = ActionManager::registerAction(m_saveAllAction, Constants::SAVEALL, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? QString() : tr("Ctrl+Shift+S")));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);
    connect(m_saveAllAction, SIGNAL(triggered()), this, SLOT(saveAll()));

    // Print Action
    icon = QIcon::fromTheme(QLatin1String("document-print"));
    tmpaction = new QAction(icon, tr("&Print..."), this);
    tmpaction->setEnabled(false);
    cmd = ActionManager::registerAction(tmpaction, Constants::PRINT, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Print);
    mfile->addAction(cmd, Constants::G_FILE_PRINT);

    // Exit Action
    icon = QIcon::fromTheme(QLatin1String("application-exit"));
    m_exitAction = new QAction(icon, tr("E&xit"), this);
    cmd = ActionManager::registerAction(m_exitAction, Constants::EXIT, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Q")));
    mfile->addAction(cmd, Constants::G_FILE_OTHER);
    connect(m_exitAction, SIGNAL(triggered()), this, SLOT(exit()));

    // Undo Action
    icon = QIcon::fromTheme(QLatin1String("edit-undo"), QIcon(QLatin1String(Constants::ICON_UNDO)));
    tmpaction = new QAction(icon, tr("&Undo"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::UNDO, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Undo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Undo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);
    tmpaction->setEnabled(false);

    // Redo Action
    icon = QIcon::fromTheme(QLatin1String("edit-redo"), QIcon(QLatin1String(Constants::ICON_REDO)));
    tmpaction = new QAction(icon, tr("&Redo"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::REDO, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Redo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Redo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);
    tmpaction->setEnabled(false);

    // Cut Action
    icon = QIcon::fromTheme(QLatin1String("edit-cut"), QIcon(QLatin1String(Constants::ICON_CUT)));
    tmpaction = new QAction(icon, tr("Cu&t"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::CUT, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Cut);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Copy Action
    icon = QIcon::fromTheme(QLatin1String("edit-copy"), QIcon(QLatin1String(Constants::ICON_COPY)));
    tmpaction = new QAction(icon, tr("&Copy"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::COPY, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Copy);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Paste Action
    icon = QIcon::fromTheme(QLatin1String("edit-paste"), QIcon(QLatin1String(Constants::ICON_PASTE)));
    tmpaction = new QAction(icon, tr("&Paste"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::PASTE, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Paste);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Select All
    icon = QIcon::fromTheme(QLatin1String("edit-select-all"));
    tmpaction = new QAction(icon, tr("Select &All"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::SELECTALL, globalContext);
    cmd->setDefaultKeySequence(QKeySequence::SelectAll);
    medit->addAction(cmd, Constants::G_EDIT_SELECTALL);
    tmpaction->setEnabled(false);

    // Goto Action
    icon = QIcon::fromTheme(QLatin1String("go-jump"));
    tmpaction = new QAction(icon, tr("&Go to Line..."), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::GOTO, globalContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+L")));
    medit->addAction(cmd, Constants::G_EDIT_OTHER);
    tmpaction->setEnabled(false);

    // Options Action
    mtools->appendGroup(Constants::G_TOOLS_OPTIONS);
    mtools->addSeparator(globalContext, Constants::G_TOOLS_OPTIONS);
    m_optionsAction = new QAction(tr("&Options..."), this);
    cmd = ActionManager::registerAction(m_optionsAction, Constants::OPTIONS, globalContext);
    if (UseMacShortcuts) {
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+,")));
        cmd->action()->setMenuRole(QAction::PreferencesRole);
    }
    mtools->addAction(cmd, Constants::G_TOOLS_OPTIONS);
    connect(m_optionsAction, SIGNAL(triggered()), this, SLOT(showOptionsDialog()));

    if (UseMacShortcuts) {
        // Minimize Action
        m_minimizeAction = new QAction(tr("Minimize"), this);
        cmd = ActionManager::registerAction(m_minimizeAction, Constants::MINIMIZE_WINDOW, globalContext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+M")));
        mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
        connect(m_minimizeAction, SIGNAL(triggered()), this, SLOT(showMinimized()));

        // Zoom Action
        m_zoomAction = new QAction(tr("Zoom"), this);
        cmd = ActionManager::registerAction(m_zoomAction, Constants::ZOOM_WINDOW, globalContext);
        mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
        connect(m_zoomAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

        // Window separator
        mwindow->addSeparator(globalContext, Constants::G_WINDOW_SIZE);
    }

    // Show Sidebar Action
    m_toggleSideBarAction = new QAction(QIcon(QLatin1String(Constants::ICON_TOGGLE_SIDEBAR)),
                                        tr("Show Sidebar"), this);
    m_toggleSideBarAction->setCheckable(true);
    cmd = ActionManager::registerAction(m_toggleSideBarAction, Constants::TOGGLE_SIDEBAR, globalContext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+0") : tr("Alt+0")));
    connect(m_toggleSideBarAction, SIGNAL(triggered(bool)), this, SLOT(setSidebarVisible(bool)));
    m_toggleSideBarButton->setDefaultAction(cmd->action());
    mwindow->addAction(cmd, Constants::G_WINDOW_VIEWS);
    m_toggleSideBarAction->setEnabled(false);


#if defined(Q_OS_MAC)
    const QString fullScreenActionText(tr("Enter Full Screen"));
    bool supportsFullScreen = MacFullScreen::supportsFullScreen();
#else
    const QString fullScreenActionText(tr("Full Screen"));
    bool supportsFullScreen = true;
#endif
    if (supportsFullScreen) {
        // Full Screen Action
        m_toggleFullScreenAction = new QAction(fullScreenActionText, this);
        m_toggleFullScreenAction->setMenuRole(QAction::NoRole);
        m_toggleFullScreenAction->setCheckable(!Utils::HostOsInfo::isMacHost());
        cmd = ActionManager::registerAction(m_toggleFullScreenAction, Constants::TOGGLE_FULLSCREEN, globalContext);
        cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Meta+F") : tr("Ctrl+Shift+F11")));
        cmd->setAttribute(Command::CA_UpdateText); /* for Mac */
        mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
        connect(m_toggleFullScreenAction, SIGNAL(triggered(bool)), this, SLOT(setFullScreen(bool)));
    }

    // Window->Views
    ActionContainer *mviews = ActionManager::createMenu(Constants::M_WINDOW_VIEWS);
    mwindow->addMenu(mviews, Constants::G_WINDOW_VIEWS);
    mviews->menu()->setTitle(tr("&Views"));

    // About IDE Action
    icon = QIcon::fromTheme(QLatin1String("help-about"));
    if (Utils::HostOsInfo::isMacHost())
        tmpaction = new QAction(icon, tr("About &Qt Creator"), this); // it's convention not to add dots to the about menu
    else
        tmpaction = new QAction(icon, tr("About &Qt Creator..."), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::ABOUT_QTCREATOR, globalContext);
    if (Utils::HostOsInfo::isMacHost())
        cmd->action()->setMenuRole(QAction::ApplicationSpecificRole);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
    connect(tmpaction, SIGNAL(triggered()), this,  SLOT(aboutQtCreator()));

    //About Plugins Action
    tmpaction = new QAction(tr("About &Plugins..."), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::ABOUT_PLUGINS, globalContext);
    if (Utils::HostOsInfo::isMacHost())
        cmd->action()->setMenuRole(QAction::ApplicationSpecificRole);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
    connect(tmpaction, SIGNAL(triggered()), this,  SLOT(aboutPlugins()));
    // About Qt Action
//    tmpaction = new QAction(tr("About &Qt..."), this);
//    cmd = ActionManager::registerAction(tmpaction, Constants:: ABOUT_QT, globalContext);
//    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
//    tmpaction->setEnabled(true);
//    connect(tmpaction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    // About sep
    if (!Utils::HostOsInfo::isMacHost()) { // doesn't have the "About" actions in the Help menu
        tmpaction = new QAction(this);
        tmpaction->setSeparator(true);
        cmd = ActionManager::registerAction(tmpaction, "QtCreator.Help.Sep.About", globalContext);
        mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    }
}

void MainWindow::newFile()
{
    showNewItemDialog(tr("New", "Title of dialog"), IWizard::allWizards(), QString());
}

void MainWindow::openFile()
{
    openFiles(editorManager()->getOpenFileNames(), ICore::SwitchMode);
}

static QList<IDocumentFactory*> getNonEditorDocumentFactories()
{
    const QList<IDocumentFactory*> allFileFactories =
        ExtensionSystem::PluginManager::getObjects<IDocumentFactory>();
    QList<IDocumentFactory*> nonEditorFileFactories;
    foreach (IDocumentFactory *factory, allFileFactories) {
        if (!qobject_cast<IEditorFactory *>(factory))
            nonEditorFileFactories.append(factory);
    }
    return nonEditorFileFactories;
}

static IDocumentFactory *findDocumentFactory(const QList<IDocumentFactory*> &fileFactories,
                                     const MimeDatabase *db,
                                     const QFileInfo &fi)
{
    if (const MimeType mt = db->findByFile(fi)) {
        const QString type = mt.type();
        foreach (IDocumentFactory *factory, fileFactories) {
            if (factory->mimeTypes().contains(type))
                return factory;
        }
    }
    return 0;
}

/*! Either opens \a fileNames with editors or loads a project.
 *
 *  \a flags can be used to stop on first failure, indicate that a file name
 *  might include line numbers and/or switch mode to edit mode.
 *
 *  \returns the first opened document. Required to support the -block flag
 *  for client mode.
 *
 *  \sa IPlugin::remoteArguments()
 */
IDocument *MainWindow::openFiles(const QStringList &fileNames, ICore::OpenFilesFlags flags)
{
    QList<IDocumentFactory*> nonEditorFileFactories = getNonEditorDocumentFactories();
    IDocument *res = 0;

    foreach (const QString &fileName, fileNames) {
        const QFileInfo fi(fileName);
        const QString absoluteFilePath = fi.absoluteFilePath();
        if (IDocumentFactory *documentFactory = findDocumentFactory(nonEditorFileFactories, mimeDatabase(), fi)) {
            IDocument *document = documentFactory->open(absoluteFilePath);
            if (!document) {
                if (flags & ICore::StopOnLoadFail)
                    return res;
            } else {
                if (!res)
                    res = document;
                if (flags & ICore::SwitchMode)
                    ModeManager::activateMode(Id(Core::Constants::MODE_EDIT));
            }
        } else {
            QFlags<EditorManager::OpenEditorFlag> emFlags;
            if (flags & ICore::SwitchMode)
                emFlags = EditorManager::ModeSwitch;
            if (flags & ICore::CanContainLineNumbers)
                emFlags |=  EditorManager::CanContainLineNumber;
            IEditor *editor = EditorManager::openEditor(absoluteFilePath, Id(), emFlags);
            if (!editor) {
                if (flags & ICore::StopOnLoadFail)
                    return res;
            } else if (!res) {
                res = editor->document();
            }
        }
    }
    return res;
}

void MainWindow::setFocusToEditor()
{
    bool focusWasMovedToEditor = false;

    // give focus to the editor if we have one
    if (IEditor *editor = EditorManager::currentEditor()) {
        if (qApp->focusWidget() != editor->widget()->focusWidget()) {
            QWidget *w = editor->widget()->focusWidget();
            if (!w)
                w = editor->widget();
            w->setFocus();
            focusWasMovedToEditor = w->hasFocus();
        }
    }

    // check for some maximized pane which we want to unmaximize
    if (OutputPanePlaceHolder::getCurrent()
        && OutputPanePlaceHolder::getCurrent()->isVisible()
        && OutputPanePlaceHolder::getCurrent()->isMaximized()) {
        OutputPanePlaceHolder::getCurrent()->unmaximize();
        return;
    }

    if (focusWasMovedToEditor)
        return;

    // check for some visible bar which we want to hide
    bool stuffVisible =
            (FindToolBarPlaceHolder::getCurrent() &&
             FindToolBarPlaceHolder::getCurrent()->isVisible())
            || (OutputPanePlaceHolder::getCurrent() &&
                OutputPanePlaceHolder::getCurrent()->isVisible())
            || (RightPanePlaceHolder::current() &&
                RightPanePlaceHolder::current()->isVisible());
    if (stuffVisible) {
        if (FindToolBarPlaceHolder::getCurrent())
            FindToolBarPlaceHolder::getCurrent()->hide();
        OutputPaneManager::instance()->slotHide();
        RightPaneWidget::instance()->setShown(false);
        return;
    }

    // switch to edit mode if necessary
    ModeManager::activateMode(Id(Constants::MODE_EDIT));
}

void MainWindow::showNewItemDialog(const QString &title,
                                          const QList<IWizard *> &wizards,
                                          const QString &defaultLocation,
                                          const QVariantMap &extraVariables)
{
    // Scan for wizards matching the filter and pick one. Don't show
    // dialog if there is only one.
    IWizard *wizard = 0;
    QString selectedPlatform;
    switch (wizards.size()) {
    case 0:
        break;
    case 1:
        wizard = wizards.front();
        break;
    default: {
        NewDialog dlg(this);
        dlg.setWizards(wizards);
        dlg.setWindowTitle(title);
        wizard = dlg.showDialog();
        selectedPlatform = dlg.selectedPlatform();
    }
        break;
    }

    if (!wizard)
        return;

    QString path = defaultLocation;
    if (path.isEmpty()) {
        switch (wizard->kind()) {
        case IWizard::ProjectWizard:
            // Project wizards: Check for projects directory or
            // use last visited directory of file dialog. Never start
            // at current.
            path = DocumentManager::useProjectsDirectory() ?
                       DocumentManager::projectsDirectory() :
                       DocumentManager::fileDialogLastVisitedDirectory();
            break;
        default:
            path = DocumentManager::fileDialogInitialDirectory();
            break;
        }
    }
    wizard->runWizard(path, this, selectedPlatform, extraVariables);
}

bool MainWindow::showOptionsDialog(Id category, Id page, QWidget *parent)
{
    emit m_coreImpl->optionsDialogRequested();
    if (!parent)
        parent = this;
    SettingsDialog *dialog = SettingsDialog::getSettingsDialog(parent, category, page);
    return dialog->execDialog();
}

void MainWindow::saveAll()
{
    DocumentManager::saveModifiedDocumentsSilently(DocumentManager::modifiedDocuments());
}

void MainWindow::exit()
{
    // this function is most likely called from a user action
    // that is from an event handler of an object
    // since on close we are going to delete everything
    // so to prevent the deleting of that object we
    // just append it
    QTimer::singleShot(0, this,  SLOT(close()));
}

void MainWindow::openFileWith()
{
    QStringList fileNames = editorManager()->getOpenFileNames();
    foreach (const QString &fileName, fileNames) {
        bool isExternal;
        const Id editorId = editorManager()->getOpenWithEditorId(fileName, &isExternal);
        if (!editorId.isValid())
            continue;
        if (isExternal)
            EditorManager::openExternalEditor(fileName, editorId);
        else
            EditorManager::openEditor(fileName, editorId, Core::EditorManager::ModeSwitch);
    }
}

ActionManager *MainWindow::actionManager() const
{
    return m_actionManager;
}

MessageManager *MainWindow::messageManager() const
{
    return m_messageManager;
}

VcsManager *MainWindow::vcsManager() const
{
    return m_vcsManager;
}

QSettings *MainWindow::settings(QSettings::Scope scope) const
{
    if (scope == QSettings::UserScope)
        return m_settings;
    else
        return m_globalSettings;
}

EditorManager *MainWindow::editorManager() const
{
    return m_editorManager;
}

ProgressManager *MainWindow::progressManager() const
{
    return m_progressManager;
}

ScriptManager *MainWindow::scriptManager() const
{
     return m_scriptManager;
}

VariableManager *MainWindow::variableManager() const
{
     return m_variableManager.data();
}

ModeManager *MainWindow::modeManager() const
{
    return m_modeManager;
}

MimeDatabase *MainWindow::mimeDatabase() const
{
    return m_mimeDatabase;
}

HelpManager *MainWindow::helpManager() const
{
    return m_helpManager;
}

IContext *MainWindow::contextObject(QWidget *widget)
{
    return m_contextWidgets.value(widget);
}

void MainWindow::addContextObject(IContext *context)
{
    if (!context)
        return;
    QWidget *widget = context->widget();
    if (m_contextWidgets.contains(widget))
        return;

    m_contextWidgets.insert(widget, context);
}

void MainWindow::removeContextObject(IContext *context)
{
    if (!context)
        return;

    QWidget *widget = context->widget();
    if (!m_contextWidgets.contains(widget))
        return;

    m_contextWidgets.remove(widget);
    if (m_activeContext == context)
        updateContextObject(0);
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            if (debugMainWindow)
                qDebug() << "main window activated";
            emit windowActivated();
        }
    } else if (e->type() == QEvent::WindowStateChange) {
        if (Utils::HostOsInfo::isMacHost()) {
            bool minimized = isMinimized();
            if (debugMainWindow)
                qDebug() << "main window state changed to minimized=" << minimized;
            m_minimizeAction->setEnabled(!minimized);
            m_zoomAction->setEnabled(!minimized);
        } else {
            bool isFullScreen = (windowState() & Qt::WindowFullScreen) != 0;
            m_toggleFullScreenAction->setChecked(isFullScreen);
        }
    }
}

void MainWindow::updateFocusWidget(QWidget *old, QWidget *now)
{
    Q_UNUSED(old)

    // Prevent changing the context object just because the menu or a menu item is activated
    if (qobject_cast<QMenuBar*>(now) || qobject_cast<QMenu*>(now))
        return;

    IContext *newContext = 0;
    if (QWidget *p = qApp->focusWidget()) {
        IContext *context = 0;
        while (p) {
            context = m_contextWidgets.value(p);
            if (context) {
                newContext = context;
                break;
            }
            p = p->parentWidget();
        }
    }

    // ignore toplevels that define no context, like popups without parent
    if (newContext || qApp->focusWidget() == focusWidget())
        updateContextObject(newContext);
}

void MainWindow::updateContextObject(IContext *context)
{
    if (context == m_activeContext)
        return;
    emit m_coreImpl->contextAboutToChange(context);
    m_activeContext = context;
    updateContext();
    if (debugMainWindow)
        qDebug() << "new context object =" << context << (context ? context->widget() : 0)
                 << (context ? context->widget()->metaObject()->className() : 0);
}

void MainWindow::resetContext()
{
    updateContextObject(0);
}

void MainWindow::aboutToShutdown()
{
    disconnect(QApplication::instance(), SIGNAL(focusChanged(QWidget*,QWidget*)),
               this, SLOT(updateFocusWidget(QWidget*,QWidget*)));
    m_activeContext = 0;
    hide();
}

static const char settingsGroup[] = "MainWindow";
static const char colorKey[] = "Color";
static const char windowGeometryKey[] = "WindowGeometry";
static const char windowStateKey[] = "WindowState";

void MainWindow::readSettings()
{
    m_settings->beginGroup(QLatin1String(settingsGroup));

    if (m_overrideColor.isValid()) {
        Utils::StyleHelper::setBaseColor(m_overrideColor);
        // Get adapted base color.
        m_overrideColor = Utils::StyleHelper::baseColor();
    } else {
        Utils::StyleHelper::setBaseColor(
                m_settings->value(QLatin1String(colorKey),
                                  QColor(Utils::StyleHelper::DEFAULT_BASE_COLOR)).value<QColor>());
    }

    if (!restoreGeometry(m_settings->value(QLatin1String(windowGeometryKey)).toByteArray()))
        resize(1008, 700); // size without window decoration
    restoreState(m_settings->value(QLatin1String(windowStateKey)).toByteArray());

    m_settings->endGroup();

    m_editorManager->readSettings();
    m_navigationWidget->restoreSettings(m_settings);
    m_rightPaneWidget->readSettings(m_settings);
}

void MainWindow::writeSettings()
{
    m_settings->beginGroup(QLatin1String(settingsGroup));

    if (!(m_overrideColor.isValid() && Utils::StyleHelper::baseColor() == m_overrideColor))
        m_settings->setValue(QLatin1String(colorKey), Utils::StyleHelper::requestedBaseColor());

    m_settings->setValue(QLatin1String(windowGeometryKey), saveGeometry());
    m_settings->setValue(QLatin1String(windowStateKey), saveState());

    m_settings->endGroup();

    DocumentManager::saveSettings();
    m_actionManager->d->saveSettings(m_settings);
    m_editorManager->saveSettings();
    m_navigationWidget->saveSettings(m_settings);
}

void MainWindow::updateAdditionalContexts(const Context &remove, const Context &add)
{
    foreach (const Id id, remove) {
        if (!id.isValid())
            continue;

        int index = m_additionalContexts.indexOf(id);
        if (index != -1)
            m_additionalContexts.removeAt(index);
    }

    foreach (const Id id, add) {
        if (!id.isValid())
            continue;

        if (!m_additionalContexts.contains(id))
            m_additionalContexts.prepend(id);
    }

    updateContext();
}

void MainWindow::updateContext()
{
    Context contexts;

    if (m_activeContext)
        contexts.add(m_activeContext->context());

    contexts.add(m_additionalContexts);

    Context uniquecontexts;
    for (int i = 0; i < contexts.size(); ++i) {
        const Id id = contexts.at(i);
        if (!uniquecontexts.contains(id))
            uniquecontexts.add(id);
    }

    m_actionManager->d->setContext(uniquecontexts);
    emit m_coreImpl->contextChanged(m_activeContext, m_additionalContexts);
}

void MainWindow::aboutToShowRecentFiles()
{
    ActionContainer *aci =
        ActionManager::actionContainer(Constants::M_FILE_RECENTFILES);
    aci->menu()->clear();

    bool hasRecentFiles = false;
    foreach (const DocumentManager::RecentFile &file, DocumentManager::recentFiles()) {
        hasRecentFiles = true;
        QAction *action = aci->menu()->addAction(
                    QDir::toNativeSeparators(Utils::withTildeHomePath(file.first)));
        action->setData(qVariantFromValue(file));
        connect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
    }
    aci->menu()->setEnabled(hasRecentFiles);

    // add the Clear Menu item
    if (hasRecentFiles) {
        aci->menu()->addSeparator();
        QAction *action = aci->menu()->addAction(QCoreApplication::translate(
                                                     "Core", Core::Constants::TR_CLEAR_MENU));
        connect(action, SIGNAL(triggered()), DocumentManager::instance(), SLOT(clearRecentFiles()));
    }
}

void MainWindow::openRecentFile()
{
    if (const QAction *action = qobject_cast<const QAction*>(sender())) {
        const DocumentManager::RecentFile file = action->data().value<DocumentManager::RecentFile>();
        EditorManager::openEditor(file.first, file.second, EditorManager::ModeSwitch);
    }
}

void MainWindow::aboutQtCreator()
{
    if (!m_versionDialog) {
        m_versionDialog = new VersionDialog(this);
        connect(m_versionDialog, SIGNAL(finished(int)),
                this, SLOT(destroyVersionDialog()));
    }
    m_versionDialog->show();
}

void MainWindow::destroyVersionDialog()
{
    if (m_versionDialog) {
        m_versionDialog->deleteLater();
        m_versionDialog = 0;
    }
}

void MainWindow::aboutPlugins()
{
    PluginDialog dialog(this);
    dialog.exec();
}

QPrinter *MainWindow::printer() const
{
    if (!m_printer)
        m_printer = new QPrinter(QPrinter::HighResolution);
    return m_printer;
}

void MainWindow::setFullScreen(bool on)
{
#if defined(Q_OS_MAC)
    Q_UNUSED(on)
    MacFullScreen::toggleFullScreen(this);
#else
    if (bool(windowState() & Qt::WindowFullScreen) == on)
        return;

    if (on) {
        setWindowState(windowState() | Qt::WindowFullScreen);
        //statusBar()->hide();
        //menuBar()->hide();
    } else {
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        //menuBar()->show();
        //statusBar()->show();
    }
#endif
}

// Display a warning with an additional button to open
// the debugger settings dialog if settingsId is nonempty.

bool MainWindow::showWarningWithOptions(const QString &title,
                                        const QString &text,
                                        const QString &details,
                                        Id settingsCategory,
                                        Id settingsId,
                                        QWidget *parent)
{
    if (parent == 0)
        parent = this;
    QMessageBox msgBox(QMessageBox::Warning, title, text,
                       QMessageBox::Ok, parent);
    if (!details.isEmpty())
        msgBox.setDetailedText(details);
    QAbstractButton *settingsButton = 0;
    if (settingsId.isValid() || settingsCategory.isValid())
        settingsButton = msgBox.addButton(tr("Settings..."), QMessageBox::AcceptRole);
    msgBox.exec();
    if (settingsButton && msgBox.clickedButton() == settingsButton)
        return showOptionsDialog(settingsCategory, settingsId);
    return false;
}
