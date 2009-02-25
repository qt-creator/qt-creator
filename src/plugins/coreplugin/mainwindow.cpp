/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "mainwindow.h"
#include "actioncontainer.h"
#include "actionmanager_p.h"
#include "basemode.h"
#include "coreimpl.h"
#include "coreconstants.h"
#include "editorgroup.h"
#include "editormanager.h"
#include "fancytabwidget.h"
#include "filemanager.h"
#include "generalsettings.h"
#include "ifilefactory.h"
#include "messagemanager.h"
#include "modemanager.h"
#include "mimedatabase.h"
#include "newdialog.h"
#include "outputpane.h"
#include "plugindialog.h"
#include "progressmanager_p.h"
#include "progressview.h"
#include "shortcutsettings.h"
#include "vcsmanager.h"

#include "scriptmanager_p.h"
#include "settingsdialog.h"
#include "stylehelper.h"
#include "variablemanager.h"
#include "versiondialog.h"
#include "viewmanager.h"
#include "uniqueidmanager.h"
#include "manhattanstyle.h"
#include "dialogs/iwizard.h"
#include "navigationwidget.h"
#include "rightpane.h"
#include "editormanager/ieditorfactory.h"
#include "baseview.h"
#include "basefilewizard.h"

#include <coreplugin/findplaceholder.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QtPlugin>
#include <QtCore/QUrl>

#include <QtGui/QApplication>
#include <QtGui/QCloseEvent>
#include <QtGui/QMenu>
#include <QtGui/QPixmap>
#include <QtGui/QPrinter>
#include <QtGui/QShortcut>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtGui/QWizard>

/*
#ifdef Q_OS_UNIX
#include <signal.h>
extern "C" void handleSigInt(int sig)
{
    Q_UNUSED(sig);
    Core::ICore::instance()->exit();
    qDebug() << "SIGINT caught. Shutting down.";
}
#endif
*/

using namespace Core;
using namespace Core::Internal;

static const char *uriListMimeFormatC = "text/uri-list";

enum { debugMainWindow = 0 };

MainWindow::MainWindow() :
    QMainWindow(),
    m_coreImpl(new CoreImpl(this)),
    m_uniqueIDManager(new UniqueIDManager()),
    m_globalContext(QList<int>() << Constants::C_GLOBAL_ID),
    m_additionalContexts(m_globalContext),
    m_settings(new QSettings(QSettings::IniFormat, QSettings::UserScope, QLatin1String("Nokia"), QLatin1String("QtCreator"), this)),
    m_printer(0),
    m_actionManager(new ActionManagerPrivate(this)),
    m_editorManager(0),
    m_fileManager(new FileManager(this)),
    m_progressManager(new ProgressManagerPrivate()),
    m_scriptManager(new ScriptManagerPrivate(this)),
    m_variableManager(new VariableManager(this)),
    m_vcsManager(new VCSManager()),
    m_viewManager(0),
    m_modeManager(0),
    m_mimeDatabase(new MimeDatabase),
    m_navigationWidget(0),
    m_rightPaneWidget(0),
    m_versionDialog(0),
    m_activeContext(0),
    m_outputMode(0),
    m_generalSettings(new GeneralSettings),
    m_shortcutSettings(new ShortcutSettings),
    m_focusToEditor(0),
    m_newAction(0),
    m_openAction(0),
    m_openWithAction(0),
    m_saveAllAction(0),
    m_exitAction(0),
    m_optionsAction(0),
    m_toggleSideBarAction(0),
    m_toggleFullScreenAction(0),
#ifdef Q_OS_MAC
    m_minimizeAction(0),
    m_zoomAction(0),
#endif
    m_toggleSideBarButton(new QToolButton)
{
    OutputPaneManager::create();

    setWindowTitle(tr("Qt Creator"));
#ifndef Q_OS_MAC
    qApp->setWindowIcon(QIcon(":/core/images/qtcreator_logo_128.png"));
#endif
    QCoreApplication::setApplicationName(QLatin1String("QtCreator"));
    QCoreApplication::setApplicationVersion(QLatin1String(Core::Constants::IDE_VERSION_LONG));
    QCoreApplication::setOrganizationName(QLatin1String("Nokia"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QString baseName = qApp->style()->objectName();
#ifdef Q_WS_X11    
    if (baseName == QLatin1String("windows")) {
        // Sometimes we get the standard windows 95 style as a fallback
        // e.g. if we are running on a KDE4 desktop
        QByteArray desktopEnvironment = qgetenv("DESKTOP_SESSION");
        if (desktopEnvironment == "kde")
            baseName = QLatin1String("plastique");
        else
            baseName = QLatin1String("cleanlooks");
    }
#endif
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
    m_modeManager->addWidget(m_progressManager->progressView());
    m_viewManager = new ViewManager(this);
    m_messageManager = new MessageManager;
    m_editorManager = new EditorManager(m_coreImpl, this);
    m_editorManager->hide();
    setCentralWidget(m_modeStack);

    connect(QApplication::instance(), SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(updateFocusWidget(QWidget*,QWidget*)));
    // Add a small Toolbutton for toggling the navigation widget
    m_toggleSideBarButton->setProperty("type", QLatin1String("dockbutton"));
    statusBar()->insertPermanentWidget(0, m_toggleSideBarButton);

//    setUnifiedTitleAndToolBarOnMac(true);
#ifdef Q_OS_UNIX
     //signal(SIGINT, handleSigInt);
#endif

    statusBar()->setProperty("p_styled", true);
    setAcceptDrops(true);
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

MainWindow::~MainWindow()
{
    hide();
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    pm->removeObject(m_shortcutSettings);
    pm->removeObject(m_generalSettings);
    delete m_messageManager;
    m_messageManager = 0;
    delete m_shortcutSettings;
    m_shortcutSettings = 0;
    delete m_generalSettings;
    m_generalSettings = 0;
    delete m_settings;
    m_settings = 0;
    delete m_printer;
    m_printer = 0;
    delete m_uniqueIDManager;
    m_uniqueIDManager = 0;
    delete m_vcsManager;
    m_vcsManager = 0;
    pm->removeObject(m_outputMode);
    delete m_outputMode;
    m_outputMode = 0;
    //we need to delete editormanager and viewmanager explicitly before the end of the destructor,
    //because they might trigger stuff that tries to access data from editorwindow, like removeContextWidget

    // All modes are now gone
    OutputPaneManager::destroy();

    // Now that the OutputPaneManager is gone, is a good time to delete the view
    pm->removeObject(m_outputView);
    delete m_outputView;

    delete m_editorManager;
    m_editorManager = 0;
    delete m_viewManager;
    m_viewManager = 0;
    delete m_progressManager;
    m_progressManager = 0;
    pm->removeObject(m_coreImpl);
    delete m_coreImpl;
    m_coreImpl = 0;

    delete m_rightPaneWidget;
    m_rightPaneWidget = 0;

    delete m_navigationWidget;
    m_navigationWidget = 0;

    delete m_modeManager;
    m_modeManager = 0;
    delete m_mimeDatabase;
    m_mimeDatabase = 0;
}

bool MainWindow::init(QString *errorMessage)
{
    Q_UNUSED(errorMessage);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    pm->addObject(m_coreImpl);
    m_viewManager->init();
    m_modeManager->init();
    m_progressManager->init();
    QWidget *outputModeWidget = new QWidget;
    outputModeWidget->setLayout(new QVBoxLayout);
    outputModeWidget->layout()->setMargin(0);
    outputModeWidget->layout()->setSpacing(0);
    m_outputMode = new BaseMode;
    m_outputMode->setName(tr("Output"));
    m_outputMode->setUniqueModeName(Constants::MODE_OUTPUT);
    m_outputMode->setIcon(QIcon(QLatin1String(":/fancyactionbar/images/mode_Output.png")));
    m_outputMode->setPriority(Constants::P_MODE_OUTPUT);
    m_outputMode->setWidget(outputModeWidget);
    OutputPanePlaceHolder *oph = new OutputPanePlaceHolder(m_outputMode);
    oph->setVisible(true);
    oph->setCloseable(false);
    outputModeWidget->layout()->addWidget(oph);
    outputModeWidget->layout()->addWidget(new Core::FindToolBarPlaceHolder(m_outputMode));
    outputModeWidget->setFocusProxy(oph);

    m_outputMode->setContext(m_globalContext);
    pm->addObject(m_outputMode);
    pm->addObject(m_generalSettings);
    pm->addObject(m_shortcutSettings);

    // Add widget to the bottom, we create the view here instead of inside the
    // OutputPaneManager, since the ViewManager needs to be initilized before
    m_outputView = new Core::BaseView;
    m_outputView->setUniqueViewName("OutputWindow");
    m_outputView->setWidget(OutputPaneManager::instance()->buttonsWidget());
    m_outputView->setDefaultPosition(Core::IView::Second);
    pm->addObject(m_outputView);
    return true;
}

void MainWindow::extensionsInitialized()
{
    m_editorManager->init();

    m_viewManager->extensionsInitalized();

    m_messageManager->init();
    OutputPaneManager::instance()->init();

    m_actionManager->initialize();
    readSettings();
    updateContext();
    show();

    emit m_coreImpl->coreOpened();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    emit m_coreImpl->saveSettingsRequested();

    // Save opened files
    bool cancelled;
    fileManager()->saveModifiedFiles(fileManager()->modifiedFiles(), &cancelled);
    if (cancelled) {
        event->ignore();
        return;
    }

    const QList<ICoreListener *> listeners =
        ExtensionSystem::PluginManager::instance()->getObjects<ICoreListener>();
    foreach (ICoreListener *listener, listeners) {
        if (!listener->coreAboutToClose()) {
            event->ignore();
            return;
        }
    }

    emit m_coreImpl->coreAboutToClose();
    writeSettings();
    event->accept();
}

// Check for desktop file manager file drop events

static bool isDesktopFileManagerDrop(const QMimeData *d, QStringList *files = 0)
{
    if (files)
        files->clear();
    // Extract dropped files from Mime data.
    if (!d->hasFormat(QLatin1String(uriListMimeFormatC)))
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
            if (files) {
                files->push_back(fileName);
            } else {
                break; // No result list, sufficient for checking
            }
        }
    }
    return hasFiles;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (isDesktopFileManagerDrop(event->mimeData())) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QStringList files;
    if (isDesktopFileManagerDrop(event->mimeData(), &files)) {
        event->accept();
        openFiles(files);
    } else {
        event->ignore();
    }
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
    ActionManagerPrivate *am = m_actionManager;

    ActionContainer *menubar = am->createMenuBar(Constants::MENU_BAR);

#ifndef Q_WS_MAC // System menu bar on Mac
    setMenuBar(menubar->menuBar());
#endif
    menubar->appendGroup(Constants::G_FILE);
    menubar->appendGroup(Constants::G_EDIT);
    menubar->appendGroup(Constants::G_VIEW);
    menubar->appendGroup(Constants::G_TOOLS);
    menubar->appendGroup(Constants::G_WINDOW);
    menubar->appendGroup(Constants::G_HELP);

    // File Menu
    ActionContainer *filemenu = am->createMenu(Constants::M_FILE);
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
    ActionContainer *medit = am->createMenu(Constants::M_EDIT);
    menubar->addMenu(medit, Constants::G_EDIT);
    medit->menu()->setTitle(tr("&Edit"));
    medit->appendGroup(Constants::G_EDIT_UNDOREDO);
    medit->appendGroup(Constants::G_EDIT_COPYPASTE);
    medit->appendGroup(Constants::G_EDIT_SELECTALL);
    medit->appendGroup(Constants::G_EDIT_ADVANCED);
    medit->appendGroup(Constants::G_EDIT_FIND);
    medit->appendGroup(Constants::G_EDIT_OTHER);

    // Tools Menu
    ActionContainer *ac = am->createMenu(Constants::M_TOOLS);
    menubar->addMenu(ac, Constants::G_TOOLS);
    ac->menu()->setTitle(tr("&Tools"));

    // Window Menu
    ActionContainer *mwindow = am->createMenu(Constants::M_WINDOW);
    menubar->addMenu(mwindow, Constants::G_WINDOW);
    mwindow->menu()->setTitle(tr("&Window"));
    mwindow->appendGroup(Constants::G_WINDOW_SIZE);
    mwindow->appendGroup(Constants::G_WINDOW_PANES);
    mwindow->appendGroup(Constants::G_WINDOW_SPLIT);
    mwindow->appendGroup(Constants::G_WINDOW_CLOSE);
    mwindow->appendGroup(Constants::G_WINDOW_NAVIGATE);
    mwindow->appendGroup(Constants::G_WINDOW_NAVIGATE_GROUPS);
    mwindow->appendGroup(Constants::G_WINDOW_OTHER);
    mwindow->appendGroup(Constants::G_WINDOW_LIST);

    // Help Menu
    ac = am->createMenu(Constants::M_HELP);
    menubar->addMenu(ac, Constants::G_HELP);
    ac->menu()->setTitle(tr("&Help"));
    ac->appendGroup(Constants::G_HELP_HELP);
    ac->appendGroup(Constants::G_HELP_ABOUT);
}

static Command *createSeparator(ActionManager *am, QObject *parent,
                                const QString &name,
                                const QList<int> &context)
{
    QAction *tmpaction = new QAction(parent);
    tmpaction->setSeparator(true);
    Command *cmd = am->registerAction(tmpaction, name, context);
    return cmd;
}

void MainWindow::registerDefaultActions()
{
    ActionManagerPrivate *am = m_actionManager;
    ActionContainer *mfile = am->actionContainer(Constants::M_FILE);
    ActionContainer *medit = am->actionContainer(Constants::M_EDIT);
    ActionContainer *mtools = am->actionContainer(Constants::M_TOOLS);
    ActionContainer *mwindow = am->actionContainer(Constants::M_WINDOW);
    ActionContainer *mhelp = am->actionContainer(Constants::M_HELP);

    // File menu separators
    Command *cmd = createSeparator(am, this, QLatin1String("QtCreator.File.Sep.Save"), m_globalContext);
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    cmd =  createSeparator(am, this, QLatin1String("QtCreator.File.Sep.Print"), m_globalContext);
    mfile->addAction(cmd, Constants::G_FILE_PRINT);

    cmd =  createSeparator(am, this, QLatin1String("QtCreator.File.Sep.Close"), m_globalContext);
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);

    cmd = createSeparator(am, this, QLatin1String("QtCreator.File.Sep.Other"), m_globalContext);
    mfile->addAction(cmd, Constants::G_FILE_OTHER);

    // Edit menu separators
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.CopyPaste"), m_globalContext);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);

    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.SelectAll"), m_globalContext);
    medit->addAction(cmd, Constants::G_EDIT_SELECTALL);

    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.Find"), m_globalContext);
    medit->addAction(cmd, Constants::G_EDIT_FIND);

    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.Advanced"), m_globalContext);
    medit->addAction(cmd, Constants::G_EDIT_ADVANCED);

    // Tools menu separators
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Tools.Sep.Options"), m_globalContext);
    mtools->addAction(cmd, Constants::G_DEFAULT_THREE);

    // Return to editor shortcut: Note this requires Qt to fix up
    // handling of shortcut overrides in menus, item views, combos....
    m_focusToEditor = new QShortcut(this);
    cmd = am->registerShortcut(m_focusToEditor, Constants::S_RETURNTOEDITOR, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_Escape));
    connect(m_focusToEditor, SIGNAL(activated()), this, SLOT(setFocusToEditor()));

    // New File Action
    m_newAction = new QAction(QIcon(Constants::ICON_NEWFILE), tr("&New..."), this);
    cmd = am->registerAction(m_newAction, Constants::NEW, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::New);
    mfile->addAction(cmd, Constants::G_FILE_NEW);
    connect(m_newAction, SIGNAL(triggered()), this, SLOT(newFile()));

    // Open Action
    m_openAction = new QAction(QIcon(Constants::ICON_OPENFILE), tr("&Open..."), this);
    cmd = am->registerAction(m_openAction, Constants::OPEN, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Open);
    mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openAction, SIGNAL(triggered()), this, SLOT(openFile()));

    // Open With Action
    m_openWithAction = new QAction(tr("&Open With..."), this);
    cmd = am->registerAction(m_openWithAction, Constants::OPEN_WITH, m_globalContext);
    mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openWithAction, SIGNAL(triggered()), this, SLOT(openFileWith()));

    // File->Recent Files Menu
    ActionContainer *ac = am->createMenu(Constants::M_FILE_RECENTFILES);
    mfile->addMenu(ac, Constants::G_FILE_OPEN);
    ac->menu()->setTitle(tr("Recent Files"));

    // Save Action
    QAction *tmpaction = new QAction(QIcon(Constants::ICON_SAVEFILE), tr("&Save"), this);
    cmd = am->registerAction(tmpaction, Constants::SAVE, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Save);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("&Save"));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // Save As Action
    tmpaction = new QAction(tr("Save &As..."), this);
    cmd = am->registerAction(tmpaction, Constants::SAVEAS, m_globalContext);
#ifdef Q_OS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+S")));
#endif
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("Save &As..."));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // SaveAll Action
    m_saveAllAction = new QAction(tr("Save A&ll"), this);
    cmd = am->registerAction(m_saveAllAction, Constants::SAVEALL, m_globalContext);
#ifndef Q_OS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+S")));
#endif
    mfile->addAction(cmd, Constants::G_FILE_SAVE);
    connect(m_saveAllAction, SIGNAL(triggered()), this, SLOT(saveAll()));

    // Print Action
    tmpaction = new QAction(tr("&Print..."), this);
    cmd = am->registerAction(tmpaction, Constants::PRINT, m_globalContext);
    mfile->addAction(cmd, Constants::G_FILE_PRINT);

    // Exit Action
    m_exitAction = new QAction(tr("E&xit"), this);
    cmd = am->registerAction(m_exitAction, Constants::EXIT, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Q")));
    mfile->addAction(cmd, Constants::G_FILE_OTHER);
    connect(m_exitAction, SIGNAL(triggered()), this, SLOT(exit()));

    // Undo Action
    tmpaction = new QAction(QIcon(Constants::ICON_UNDO), tr("&Undo"), this);
    cmd = am->registerAction(tmpaction, Constants::UNDO, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Undo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("&Undo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);

    // Redo Action
    tmpaction = new QAction(QIcon(Constants::ICON_REDO), tr("&Redo"), this);
    cmd = am->registerAction(tmpaction, Constants::REDO, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Redo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("&Redo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);

    // Cut Action
    tmpaction = new QAction(QIcon(Constants::ICON_CUT), tr("Cu&t"), this);
    cmd = am->registerAction(tmpaction, Constants::CUT, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Cut);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);

    // Copy Action
    tmpaction = new QAction(QIcon(Constants::ICON_COPY), tr("&Copy"), this);
    cmd = am->registerAction(tmpaction, Constants::COPY, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Copy);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);

    // Paste Action
    tmpaction = new QAction(QIcon(Constants::ICON_PASTE), tr("&Paste"), this);
    cmd = am->registerAction(tmpaction, Constants::PASTE, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::Paste);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);

    // Select All
    tmpaction = new QAction(tr("&Select All"), this);
    cmd = am->registerAction(tmpaction, Constants::SELECTALL, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence::SelectAll);
    medit->addAction(cmd, Constants::G_EDIT_SELECTALL);

    // Goto Action
    tmpaction = new QAction(tr("&Go To Line..."), this);
    cmd = am->registerAction(tmpaction, Constants::GOTO, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+L")));
    medit->addAction(cmd, Constants::G_EDIT_OTHER);

    // Options Action
    m_optionsAction = new QAction(tr("&Options..."), this);
    cmd = am->registerAction(m_optionsAction, Constants::OPTIONS, m_globalContext);
#ifdef Q_OS_MAC
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+,"));
#endif
    mtools->addAction(cmd, Constants::G_DEFAULT_THREE);
    connect(m_optionsAction, SIGNAL(triggered()), this, SLOT(showOptionsDialog()));

#ifdef Q_OS_MAC
    // Minimize Action
    m_minimizeAction = new QAction(tr("Minimize"), this);
    cmd = am->registerAction(m_minimizeAction, Constants::MINIMIZE_WINDOW, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+M"));
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
    connect(m_minimizeAction, SIGNAL(triggered()), this, SLOT(showMinimized()));

    // Zoom Action
    m_zoomAction = new QAction(tr("Zoom"), this);
    cmd = am->registerAction(m_zoomAction, Constants::ZOOM_WINDOW, m_globalContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
    connect(m_zoomAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

    // Window separator
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Window.Sep.Size"), m_globalContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
#endif

    // Show Sidebar Action
    m_toggleSideBarAction = new QAction(QIcon(Constants::ICON_TOGGLE_SIDEBAR),
                                        tr("Show Sidebar"), this);
    m_toggleSideBarAction->setCheckable(true);
    cmd = am->registerAction(m_toggleSideBarAction, Constants::TOGGLE_SIDEBAR, m_globalContext);
#ifdef Q_OS_MAC
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+0"));
#else
    cmd->setDefaultKeySequence(QKeySequence("Alt+0"));
#endif
    connect(m_toggleSideBarAction, SIGNAL(triggered(bool)), this, SLOT(setSidebarVisible(bool)));
    m_toggleSideBarButton->setDefaultAction(cmd->action());
    mwindow->addAction(cmd, Constants::G_WINDOW_PANES);
    m_toggleSideBarAction->setEnabled(false);

#if !defined(Q_OS_MAC)
    // Full Screen Action
    m_toggleFullScreenAction = new QAction(tr("Full Screen"), this);
    m_toggleFullScreenAction->setCheckable(true);
    cmd = am->registerAction(m_toggleFullScreenAction, Constants::TOGGLE_FULLSCREEN, m_globalContext);
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+Shift+F11"));
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
    connect(m_toggleFullScreenAction, SIGNAL(triggered(bool)), this, SLOT(setFullScreen(bool)));
#endif

    // About IDE Action
#ifdef Q_OS_MAC
    tmpaction = new QAction(tr("About &Qt Creator"), this); // it's convention not to add dots to the about menu
#else
    tmpaction = new QAction(tr("About &Qt Creator..."), this);
#endif
    cmd = am->registerAction(tmpaction, Constants:: ABOUT_WORKBENCH, m_globalContext);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
    connect(tmpaction, SIGNAL(triggered()), this,  SLOT(aboutQtCreator()));
    //About Plugins Action
    tmpaction = new QAction(tr("About &Plugins..."), this);
    cmd = am->registerAction(tmpaction, Constants::ABOUT_PLUGINS, m_globalContext);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
#ifdef Q_OS_MAC
    cmd->action()->setMenuRole(QAction::ApplicationSpecificRole);
#endif
    connect(tmpaction, SIGNAL(triggered()), this,  SLOT(aboutPlugins()));
    // About Qt Action
//    tmpaction = new QAction(tr("About &Qt..."), this);
//    cmd = am->registerAction(tmpaction, Constants:: ABOUT_QT, m_globalContext);
//    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
//    tmpaction->setEnabled(true);
//    connect(tmpaction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    // About sep
    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    cmd = am->registerAction(tmpaction, QLatin1String("QtCreator.Help.Sep.About"), m_globalContext);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
}

void MainWindow::newFile()
{
    showNewItemDialog(tr("New", "Title of dialog"),BaseFileWizard::allWizards());
}

void MainWindow::openFile()
{
    openFiles(editorManager()->getOpenFileNames());
}

static QList<IFileFactory*> getNonEditorFileFactories()
{
    const QList<IFileFactory*> allFileFactories =
        ExtensionSystem::PluginManager::instance()->getObjects<IFileFactory>();
    QList<IFileFactory*> nonEditorFileFactories;
    foreach (IFileFactory *factory, allFileFactories) {
        if (!qobject_cast<IEditorFactory *>(factory))
            nonEditorFileFactories.append(factory);
    }
    return nonEditorFileFactories;
}

static IFileFactory *findFileFactory(const QList<IFileFactory*> &fileFactories,
                                     const MimeDatabase *db,
                                     const QFileInfo &fi)
{
    if (const MimeType mt = db->findByFile(fi)) {
        const QString type = mt.type();
        foreach (IFileFactory *factory, fileFactories) {
            if (factory->mimeTypes().contains(type))
                return factory;
        }
    }
    return 0;
}

// opens either an editor or loads a project
void MainWindow::openFiles(const QStringList &fileNames)
{
    bool needToSwitchToEditor = false;
    QList<IFileFactory*> nonEditorFileFactories = getNonEditorFileFactories();

    foreach (const QString &fileName, fileNames) {
        const QFileInfo fi(fileName);
        const QString absoluteFilePath = fi.absoluteFilePath();
        if (IFileFactory *fileFactory = findFileFactory(nonEditorFileFactories, mimeDatabase(), fi)) {
            fileFactory->open(absoluteFilePath);
        } else {
            IEditor *editor = editorManager()->openEditor(absoluteFilePath);
            if (editor)
                needToSwitchToEditor = true;
        }
    }
    if (needToSwitchToEditor)
        editorManager()->ensureEditorManagerVisible();
}

void MainWindow::setFocusToEditor()
{
    QWidget *focusWidget = qApp->focusWidget();
    // ### Duplicated code from EditMode::makeSureEditorManagerVisible
    IMode *currentMode = m_coreImpl->modeManager()->currentMode();
    if (currentMode && currentMode->uniqueModeName() != QLatin1String(Constants::MODE_EDIT) &&
        currentMode->uniqueModeName() != QLatin1String("GdbDebugger.Mode.Debug"))
    {
        m_coreImpl->modeManager()->activateMode(QLatin1String(Constants::MODE_EDIT));
    }

    EditorGroup *group = m_editorManager->currentEditorGroup();
    if (group && group->widget())
        group->widget()->setFocus();
    if (focusWidget && focusWidget == qApp->focusWidget()) {
        if (FindToolBarPlaceHolder::getCurrent())
            FindToolBarPlaceHolder::getCurrent()->hide();
        OutputPaneManager::instance()->slotHide();
        RightPaneWidget::instance()->setShown(false);
    }
}

QStringList MainWindow::showNewItemDialog(const QString &title,
                                          const QList<IWizard *> &wizards,
                                          const QString &defaultLocation)
{
    QString defaultDir = defaultLocation;
    if (defaultDir.isEmpty()) {
        if (!m_coreImpl->fileManager()->currentFile().isEmpty())
            defaultDir = QFileInfo(m_coreImpl->fileManager()->currentFile()).absolutePath();
    }

    // Scan for wizards matching the filter and pick one. Don't show
    // dialog if there is only one.
    IWizard *wizard = 0;
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
    }
        break;
    }

    if (!wizard)
        return QStringList();
    return wizard->runWizard(defaultDir, this);
}

void MainWindow::showOptionsDialog(const QString &category, const QString &page)
{
    emit m_coreImpl->optionsDialogRequested();
    SettingsDialog dlg(this, category, page);
    dlg.exec();
}

void MainWindow::saveAll()
{
    m_fileManager->saveModifiedFiles(m_fileManager->modifiedFiles());
    emit m_coreImpl->saveSettingsRequested();
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
        const QString editorKind = editorManager()->getOpenWithEditorKind(fileName);
        if (editorKind.isEmpty())
            continue;
        editorManager()->openEditor(fileName, editorKind);
    }
}

ActionManager *MainWindow::actionManager() const
{
    return m_actionManager;
}

FileManager *MainWindow::fileManager() const
{
    return m_fileManager;
}

UniqueIDManager *MainWindow::uniqueIDManager() const
{
    return m_uniqueIDManager;
}

MessageManager *MainWindow::messageManager() const
{
    return m_messageManager;
}

VCSManager *MainWindow::vcsManager() const
{
    return m_vcsManager;
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
     return m_variableManager;
}

ModeManager *MainWindow::modeManager() const
{
    return m_modeManager;
}

MimeDatabase *MainWindow::mimeDatabase() const
{
    return m_mimeDatabase;
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
    if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            if (debugMainWindow)
                qDebug() << "main window activated";
            emit windowActivated();
        }
    } else if (e->type() == QEvent::WindowStateChange) {
#ifdef Q_OS_MAC
        bool minimized = isMinimized();
        if (debugMainWindow)
            qDebug() << "main window state changed to minimized=" << minimized;
        m_minimizeAction->setEnabled(!minimized);
        m_zoomAction->setEnabled(!minimized);
#else
        bool isFullScreen = (windowState() & Qt::WindowFullScreen) != 0;
        m_toggleFullScreenAction->setChecked(isFullScreen);
#endif
    }
}

void MainWindow::updateFocusWidget(QWidget *old, QWidget *now)
{
    Q_UNUSED(old)
    Q_UNUSED(now)
    if (focusWidget())    {
        IContext *context = 0;
        QWidget *p = focusWidget();
        while (p) {
            context = m_contextWidgets.value(p);
            if (context) {
                if (m_activeContext != context)
                    updateContextObject(context);
                break;
            }
            p = p->parentWidget();
        }
    }
}

void MainWindow::updateContextObject(IContext  *context)
{
    IContext *oldContext = m_activeContext;
    m_activeContext = context;
    if (!context || oldContext != m_activeContext) {
        emit m_coreImpl->contextAboutToChange(context);
        updateContext();
        if (debugMainWindow)
            qDebug() << "new context object =" << context << (context ? context->widget() : 0)
                     << (context ? context->widget()->metaObject()->className() : 0);
        emit m_coreImpl->contextChanged(context);
    }
}

void MainWindow::resetContext()
{
    updateContextObject(0);
}

static const char *settingsGroup = "MainWindow";
static const char *geometryKey = "Geometry";
static const char *colorKey = "Color";
static const char *maxKey = "Maximized";

void MainWindow::readSettings()
{
    m_settings->beginGroup(QLatin1String(settingsGroup));
    StyleHelper::setBaseColor(m_settings->value(QLatin1String(colorKey)).value<QColor>());
    const QVariant geom = m_settings->value(QLatin1String(geometryKey));
    if (geom.isValid()) {
        setGeometry(geom.toRect());
    } else {
        resize(1024, 700);
    }
    if (m_settings->value(QLatin1String(maxKey), false).toBool())
        setWindowState(Qt::WindowMaximized);

    m_settings->endGroup();
    m_editorManager->readSettings(m_settings);
    m_navigationWidget->restoreSettings(m_settings);
    m_rightPaneWidget->readSettings(m_settings);
}

void MainWindow::writeSettings()
{
    m_settings->beginGroup(QLatin1String(settingsGroup));
    m_settings->setValue(colorKey, StyleHelper::baseColor());
    const QString maxSettingsKey = QLatin1String(maxKey);
    if (windowState() & Qt::WindowMaximized) {
        m_settings->setValue(maxSettingsKey, true);
    } else {
        m_settings->setValue(maxSettingsKey, false);
        m_settings->setValue(QLatin1String(geometryKey), geometry());
    }
    m_settings->endGroup();

    m_fileManager->saveRecentFiles();
    m_viewManager->saveSettings(m_settings);
    m_actionManager->saveSettings(m_settings);
    m_editorManager->saveSettings(m_settings);
    m_navigationWidget->saveSettings(m_settings);
}

void MainWindow::addAdditionalContext(int context)
{
    if (context == 0)
        return;

    if (!m_additionalContexts.contains(context))
        m_additionalContexts.prepend(context);
}

void MainWindow::removeAdditionalContext(int context)
{
    if (context == 0)
        return;

    int index = m_additionalContexts.indexOf(context);
    if (index != -1)
        m_additionalContexts.removeAt(index);
}

bool MainWindow::hasContext(int context) const
{
    return m_actionManager->hasContext(context);
}

void MainWindow::updateContext()
{
    QList<int> contexts;

    if (m_activeContext)
        contexts += m_activeContext->context();
    IEditor *editor = m_editorManager->currentEditor();
    if (editor && (EditorManagerPlaceHolder::current() != 0)) {
        contexts += editor->context();
    }

    contexts += m_additionalContexts;

    QList<int> uniquecontexts;
    for (int i = 0; i < contexts.size(); ++i) {
        const int c = contexts.at(i);
        if (!uniquecontexts.contains(c))
            uniquecontexts << c;
    }

    m_actionManager->setContext(uniquecontexts);
}

void MainWindow::aboutToShowRecentFiles()
{
    ActionContainer *aci =
        m_actionManager->actionContainer(Constants::M_FILE_RECENTFILES);
    aci->menu()->clear();
    m_recentFilesActions.clear();

    bool hasRecentFiles = false;
    foreach (QString s, m_fileManager->recentFiles()) {
        hasRecentFiles = true;
        QAction *action = aci->menu()->addAction(s);
        m_recentFilesActions.insert(action, s);
        connect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
    }
    aci->menu()->setEnabled(hasRecentFiles);
}

void MainWindow::openRecentFile()
{
    QAction *a = qobject_cast<QAction*>(sender());
    if (m_recentFilesActions.contains(a)) {
        editorManager()->openEditor(m_recentFilesActions.value(a));
        editorManager()->ensureEditorManagerVisible();
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
    return  m_printer;
}

void MainWindow::setFullScreen(bool on)
{
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
}
