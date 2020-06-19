/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "mainwindow.h"

#include "icore.h"
#include "jsexpander.h"
#include "mimetypesettings.h"
#include "fancytabwidget.h"
#include "documentmanager.h"
#include "generalsettings.h"
#include "idocumentfactory.h"
#include "messagemanager.h"
#include "modemanager.h"
#include "outputpanemanager.h"
#include "plugindialog.h"
#include "vcsmanager.h"
#include "versiondialog.h"
#include "statusbarmanager.h"
#include "id.h"
#include "manhattanstyle.h"
#include "navigationwidget.h"
#include "rightpane.h"
#include "editormanager/ieditorfactory.h"
#include "systemsettings.h"
#include "externaltoolmanager.h"
#include "editormanager/systemeditor.h"
#include "windowsupport.h"
#include "coreicons.h"

#include <app/app_version.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actionmanager_p.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/dialogs/externaltoolconfig.h>
#include <coreplugin/dialogs/newdialog.h>
#include <coreplugin/dialogs/shortcutsettings.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/editormanager_p.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <coreplugin/progressmanager/progressmanager_p.h>
#include <coreplugin/progressmanager/progressview.h>
#include <coreplugin/settingsdatabase.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/historycompleter.h>
#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMenuBar>
#include <QPrinter>
#include <QSettings>
#include <QStatusBar>
#include <QStyleFactory>
#include <QTimer>
#include <QToolButton>
#include <QUrl>

using namespace ExtensionSystem;
using namespace Utils;

namespace Core {
namespace Internal {

enum { debugMainWindow = 0 };

MainWindow::MainWindow()
    : AppMainWindow()
    , m_coreImpl(new ICore(this))
    , m_lowPrioAdditionalContexts(Constants::C_GLOBAL)
    , m_settingsDatabase(
          new SettingsDatabase(QFileInfo(PluginManager::settings()->fileName()).path(),
                               QLatin1String(Constants::IDE_CASED_ID),
                               this))
    , m_progressManager(new ProgressManagerPrivate)
    , m_jsExpander(JsExpander::createGlobalJsExpander())
    , m_vcsManager(new VcsManager)
    , m_modeStack(new FancyTabWidget(this))
    , m_generalSettings(new GeneralSettings)
    , m_systemSettings(new SystemSettings)
    , m_shortcutSettings(new ShortcutSettings)
    , m_toolSettings(new ToolSettings)
    , m_mimeTypeSettings(new MimeTypeSettings)
    , m_systemEditor(new SystemEditor)
    , m_toggleLeftSideBarButton(new QToolButton)
    , m_toggleRightSideBarButton(new QToolButton)
{
    (void) new DocumentManager(this);

    HistoryCompleter::setSettings(PluginManager::settings());

    setWindowTitle(Constants::IDE_DISPLAY_NAME);
    if (HostOsInfo::isLinuxHost())
        QApplication::setWindowIcon(Icons::QTCREATORLOGO_BIG.icon());
    QString baseName = QApplication::style()->objectName();
    // Sometimes we get the standard windows 95 style as a fallback
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost()
            && baseName == QLatin1String("windows")) {
        baseName = QLatin1String("fusion");
    }

    // if the user has specified as base style in the theme settings,
    // prefer that
    const QStringList available = QStyleFactory::keys();
    foreach (const QString &s, Utils::creatorTheme()->preferredStyles()) {
        if (available.contains(s, Qt::CaseInsensitive)) {
            baseName = s;
            break;
        }
    }

    QApplication::setStyle(new ManhattanStyle(baseName));
    m_generalSettings->setShowShortcutsInContextMenu(
        m_generalSettings->showShortcutsInContextMenu());

    setDockNestingEnabled(true);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    m_modeManager = new ModeManager(this, m_modeStack);
    connect(m_modeStack, &FancyTabWidget::topAreaClicked, this, [](Qt::MouseButton, Qt::KeyboardModifiers modifiers) {
        if (modifiers & Qt::ShiftModifier) {
            QColor color = QColorDialog::getColor(StyleHelper::requestedBaseColor(), ICore::dialogParent());
            if (color.isValid())
                StyleHelper::setBaseColor(color);
        }
    });

    registerDefaultContainers();
    registerDefaultActions();

    m_leftNavigationWidget = new NavigationWidget(m_toggleLeftSideBarAction, Side::Left);
    m_rightNavigationWidget = new NavigationWidget(m_toggleRightSideBarAction, Side::Right);
    m_rightPaneWidget = new RightPaneWidget();

    m_messageManager = new MessageManager;
    m_editorManager = new EditorManager(this);
    m_externalToolManager = new ExternalToolManager();
    setCentralWidget(m_modeStack);

    m_progressManager->progressView()->setParent(this);

    connect(qApp, &QApplication::focusChanged, this, &MainWindow::updateFocusWidget);

    // Add small Toolbuttons for toggling the navigation widgets
    StatusBarManager::addStatusBarWidget(m_toggleLeftSideBarButton, StatusBarManager::First);
    int childsCount = statusBar()->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly).count();
    statusBar()->insertPermanentWidget(childsCount - 1, m_toggleRightSideBarButton); // before QSizeGrip

//    setUnifiedTitleAndToolBarOnMac(true);
    //if (HostOsInfo::isAnyUnixHost())
        //signal(SIGINT, handleSigInt);

    statusBar()->setProperty("p_styled", true);

    auto dropSupport = new DropSupport(this, [](QDropEvent *event, DropSupport *) {
        return event->source() == nullptr; // only accept drops from the "outside" (e.g. file manager)
    });
    connect(dropSupport, &DropSupport::filesDropped,
            this, &MainWindow::openDroppedFiles);
}

NavigationWidget *MainWindow::navigationWidget(Side side) const
{
    return side == Side::Left ? m_leftNavigationWidget : m_rightNavigationWidget;
}

void MainWindow::setSidebarVisible(bool visible, Side side)
{
    if (NavigationWidgetPlaceHolder::current(side))
        navigationWidget(side)->setShown(visible);
}

void MainWindow::setOverrideColor(const QColor &color)
{
    m_overrideColor = color;
}

QStringList MainWindow::additionalAboutInformation() const
{
    return m_aboutInformation;
}

void MainWindow::appendAboutInformation(const QString &line)
{
    m_aboutInformation.append(line);
}

void MainWindow::addPreCloseListener(const std::function<bool ()> &listener)
{
    m_preCloseListeners.append(listener);
}

MainWindow::~MainWindow()
{
    // explicitly delete window support, because that calls methods from ICore that call methods
    // from mainwindow, so mainwindow still needs to be alive
    delete m_windowSupport;
    m_windowSupport = nullptr;

    delete m_externalToolManager;
    m_externalToolManager = nullptr;
    delete m_messageManager;
    m_messageManager = nullptr;
    delete m_shortcutSettings;
    m_shortcutSettings = nullptr;
    delete m_generalSettings;
    m_generalSettings = nullptr;
    delete m_systemSettings;
    m_systemSettings = nullptr;
    delete m_toolSettings;
    m_toolSettings = nullptr;
    delete m_mimeTypeSettings;
    m_mimeTypeSettings = nullptr;
    delete m_systemEditor;
    m_systemEditor = nullptr;
    delete m_printer;
    m_printer = nullptr;
    delete m_vcsManager;
    m_vcsManager = nullptr;
    //we need to delete editormanager and statusbarmanager explicitly before the end of the destructor,
    //because they might trigger stuff that tries to access data from editorwindow, like removeContextWidget

    // All modes are now gone
    OutputPaneManager::destroy();

    delete m_leftNavigationWidget;
    delete m_rightNavigationWidget;
    m_leftNavigationWidget = nullptr;
    m_rightNavigationWidget = nullptr;

    delete m_editorManager;
    m_editorManager = nullptr;
    delete m_progressManager;
    m_progressManager = nullptr;

    delete m_coreImpl;
    m_coreImpl = nullptr;

    delete m_rightPaneWidget;
    m_rightPaneWidget = nullptr;

    delete m_modeManager;
    m_modeManager = nullptr;

    delete m_jsExpander;
    m_jsExpander = nullptr;
}

void MainWindow::init()
{
    m_progressManager->init(); // needs the status bar manager
    MessageManager::init();
}

void MainWindow::extensionsInitialized()
{
    EditorManagerPrivate::extensionsInitialized();
    MimeTypeSettings::restoreSettings();
    m_windowSupport = new WindowSupport(this, Context("Core.MainWindow"));
    m_windowSupport->setCloseActionEnabled(false);
    OutputPaneManager::create();
    m_vcsManager->extensionsInitialized();
    m_leftNavigationWidget->setFactories(INavigationWidgetFactory::allNavigationFactories());
    m_rightNavigationWidget->setFactories(INavigationWidgetFactory::allNavigationFactories());

    ModeManager::extensionsInitialized();

    readSettings();
    updateContext();

    emit m_coreImpl->coreAboutToOpen();
    // Delay restoreWindowState, since it is overridden by LayoutRequest event
    QTimer::singleShot(0, this, &MainWindow::restoreWindowState);
    QTimer::singleShot(0, m_coreImpl, &ICore::coreOpened);
}

static void setRestart(bool restart)
{
    qApp->setProperty("restart", restart);
}

void MainWindow::restart()
{
    setRestart(true);
    exit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    const auto cancelClose = [event] {
        event->ignore();
        setRestart(false);
    };

    // work around QTBUG-43344
    static bool alreadyClosed = false;
    if (alreadyClosed) {
        event->accept();
        return;
    }

    ICore::saveSettings(ICore::MainWindowClosing);

    // Save opened files
    if (!DocumentManager::saveAllModifiedDocuments()) {
        cancelClose();
        return;
    }

    foreach (const std::function<bool()> &listener, m_preCloseListeners) {
        if (!listener()) {
            cancelClose();
            return;
        }
    }

    emit m_coreImpl->coreAboutToClose();

    saveWindowSettings();

    m_leftNavigationWidget->closeSubWidgets();
    m_rightNavigationWidget->closeSubWidgets();

    event->accept();
    alreadyClosed = true;
}

void MainWindow::openDroppedFiles(const QList<DropSupport::FileSpec> &files)
{
    raiseWindow();
    QStringList filePaths = Utils::transform(files, &DropSupport::FileSpec::filePath);
    openFiles(filePaths, ICore::SwitchMode);
}

IContext *MainWindow::currentContextObject() const
{
    return m_activeContext.isEmpty() ? nullptr : m_activeContext.first();
}

QStatusBar *MainWindow::statusBar() const
{
    return m_modeStack->statusBar();
}

InfoBar *MainWindow::infoBar() const
{
    return m_modeStack->infoBar();
}

void MainWindow::registerDefaultContainers()
{
    ActionContainer *menubar = ActionManager::createMenuBar(Constants::MENU_BAR);

    if (!HostOsInfo::isMacHost()) // System menu bar on Mac
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
    filemenu->appendGroup(Constants::G_FILE_EXPORT);
    filemenu->appendGroup(Constants::G_FILE_CLOSE);
    filemenu->appendGroup(Constants::G_FILE_PRINT);
    filemenu->appendGroup(Constants::G_FILE_OTHER);
    connect(filemenu->menu(), &QMenu::aboutToShow, this, &MainWindow::aboutToShowRecentFiles);


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

    ActionContainer *mview = ActionManager::createMenu(Constants::M_VIEW);
    menubar->addMenu(mview, Constants::G_VIEW);
    mview->menu()->setTitle(tr("&View"));
    mview->appendGroup(Constants::G_VIEW_VIEWS);
    mview->appendGroup(Constants::G_VIEW_PANES);

    // Tools Menu
    ActionContainer *ac = ActionManager::createMenu(Constants::M_TOOLS);
    menubar->addMenu(ac, Constants::G_TOOLS);
    ac->menu()->setTitle(tr("&Tools"));

    // Window Menu
    ActionContainer *mwindow = ActionManager::createMenu(Constants::M_WINDOW);
    menubar->addMenu(mwindow, Constants::G_WINDOW);
    mwindow->menu()->setTitle(tr("&Window"));
    mwindow->appendGroup(Constants::G_WINDOW_SIZE);
    mwindow->appendGroup(Constants::G_WINDOW_SPLIT);
    mwindow->appendGroup(Constants::G_WINDOW_NAVIGATE);
    mwindow->appendGroup(Constants::G_WINDOW_LIST);
    mwindow->appendGroup(Constants::G_WINDOW_OTHER);

    // Help Menu
    ac = ActionManager::createMenu(Constants::M_HELP);
    menubar->addMenu(ac, Constants::G_HELP);
    ac->menu()->setTitle(tr("&Help"));
    ac->appendGroup(Constants::G_HELP_HELP);
    ac->appendGroup(Constants::G_HELP_SUPPORT);
    ac->appendGroup(Constants::G_HELP_ABOUT);
    ac->appendGroup(Constants::G_HELP_UPDATES);

    // macOS touch bar
    ac = ActionManager::createTouchBar(Constants::TOUCH_BAR,
                                       QIcon(),
                                       "Main TouchBar" /*never visible*/);
    ac->appendGroup(Constants::G_TOUCHBAR_HELP);
    ac->appendGroup(Constants::G_TOUCHBAR_EDITOR);
    ac->appendGroup(Constants::G_TOUCHBAR_NAVIGATION);
    ac->appendGroup(Constants::G_TOUCHBAR_OTHER);
    ac->touchBar()->setApplicationTouchBar();
}

void MainWindow::registerDefaultActions()
{
    ActionContainer *mfile = ActionManager::actionContainer(Constants::M_FILE);
    ActionContainer *medit = ActionManager::actionContainer(Constants::M_EDIT);
    ActionContainer *mview = ActionManager::actionContainer(Constants::M_VIEW);
    ActionContainer *mtools = ActionManager::actionContainer(Constants::M_TOOLS);
    ActionContainer *mwindow = ActionManager::actionContainer(Constants::M_WINDOW);
    ActionContainer *mhelp = ActionManager::actionContainer(Constants::M_HELP);

    // File menu separators
    mfile->addSeparator(Constants::G_FILE_SAVE);
    mfile->addSeparator(Constants::G_FILE_EXPORT);
    mfile->addSeparator(Constants::G_FILE_PRINT);
    mfile->addSeparator(Constants::G_FILE_CLOSE);
    mfile->addSeparator(Constants::G_FILE_OTHER);
    // Edit menu separators
    medit->addSeparator(Constants::G_EDIT_COPYPASTE);
    medit->addSeparator(Constants::G_EDIT_SELECTALL);
    medit->addSeparator(Constants::G_EDIT_FIND);
    medit->addSeparator(Constants::G_EDIT_ADVANCED);

    // Return to editor shortcut: Note this requires Qt to fix up
    // handling of shortcut overrides in menus, item views, combos....
    m_focusToEditor = new QAction(tr("Return to Editor"), this);
    Command *cmd = ActionManager::registerAction(m_focusToEditor, Constants::S_RETURNTOEDITOR);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_Escape));
    connect(m_focusToEditor, &QAction::triggered, this, &MainWindow::setFocusToEditor);

    // New File Action
    QIcon icon = QIcon::fromTheme(QLatin1String("document-new"), Utils::Icons::NEWFILE.icon());
    m_newAction = new QAction(icon, tr("&New File or Project..."), this);
    cmd = ActionManager::registerAction(m_newAction, Constants::NEW);
    cmd->setDefaultKeySequence(QKeySequence::New);
    mfile->addAction(cmd, Constants::G_FILE_NEW);
    connect(m_newAction, &QAction::triggered, this, []() {
        if (!ICore::isNewItemDialogRunning()) {
            ICore::showNewItemDialog(tr("New File or Project", "Title of dialog"),
                                     IWizardFactory::allWizardFactories(), QString());
        } else {
            ICore::raiseWindow(ICore::newItemDialog());
        }
    });

    // Open Action
    icon = QIcon::fromTheme(QLatin1String("document-open"), Utils::Icons::OPENFILE.icon());
    m_openAction = new QAction(icon, tr("&Open File or Project..."), this);
    cmd = ActionManager::registerAction(m_openAction, Constants::OPEN);
    cmd->setDefaultKeySequence(QKeySequence::Open);
    mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFile);

    // Open With Action
    m_openWithAction = new QAction(tr("Open File &With..."), this);
    cmd = ActionManager::registerAction(m_openWithAction, Constants::OPEN_WITH);
    mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openWithAction, &QAction::triggered, this, &MainWindow::openFileWith);

    // File->Recent Files Menu
    ActionContainer *ac = ActionManager::createMenu(Constants::M_FILE_RECENTFILES);
    mfile->addMenu(ac, Constants::G_FILE_OPEN);
    ac->menu()->setTitle(tr("Recent &Files"));
    ac->setOnAllDisabledBehavior(ActionContainer::Show);

    // Save Action
    icon = QIcon::fromTheme(QLatin1String("document-save"), Utils::Icons::SAVEFILE.icon());
    QAction *tmpaction = new QAction(icon, EditorManager::tr("&Save"), this);
    tmpaction->setEnabled(false);
    cmd = ActionManager::registerAction(tmpaction, Constants::SAVE);
    cmd->setDefaultKeySequence(QKeySequence::Save);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Save"));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // Save As Action
    icon = QIcon::fromTheme(QLatin1String("document-save-as"));
    tmpaction = new QAction(icon, EditorManager::tr("Save &As..."), this);
    tmpaction->setEnabled(false);
    cmd = ActionManager::registerAction(tmpaction, Constants::SAVEAS);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Ctrl+Shift+S") : QString()));
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Save As..."));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // SaveAll Action
    DocumentManager::registerSaveAllAction();

    // Print Action
    icon = QIcon::fromTheme(QLatin1String("document-print"));
    tmpaction = new QAction(icon, tr("&Print..."), this);
    tmpaction->setEnabled(false);
    cmd = ActionManager::registerAction(tmpaction, Constants::PRINT);
    cmd->setDefaultKeySequence(QKeySequence::Print);
    mfile->addAction(cmd, Constants::G_FILE_PRINT);

    // Exit Action
    icon = QIcon::fromTheme(QLatin1String("application-exit"));
    m_exitAction = new QAction(icon, tr("E&xit"), this);
    m_exitAction->setMenuRole(QAction::QuitRole);
    cmd = ActionManager::registerAction(m_exitAction, Constants::EXIT);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Q")));
    mfile->addAction(cmd, Constants::G_FILE_OTHER);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::exit);

    // Undo Action
    icon = QIcon::fromTheme(QLatin1String("edit-undo"), Utils::Icons::UNDO.icon());
    tmpaction = new QAction(icon, tr("&Undo"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::UNDO);
    cmd->setDefaultKeySequence(QKeySequence::Undo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Undo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);
    tmpaction->setEnabled(false);

    // Redo Action
    icon = QIcon::fromTheme(QLatin1String("edit-redo"), Utils::Icons::REDO.icon());
    tmpaction = new QAction(icon, tr("&Redo"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::REDO);
    cmd->setDefaultKeySequence(QKeySequence::Redo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Redo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);
    tmpaction->setEnabled(false);

    // Cut Action
    icon = QIcon::fromTheme(QLatin1String("edit-cut"), Utils::Icons::CUT.icon());
    tmpaction = new QAction(icon, tr("Cu&t"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::CUT);
    cmd->setDefaultKeySequence(QKeySequence::Cut);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Copy Action
    icon = QIcon::fromTheme(QLatin1String("edit-copy"), Utils::Icons::COPY.icon());
    tmpaction = new QAction(icon, tr("&Copy"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::COPY);
    cmd->setDefaultKeySequence(QKeySequence::Copy);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Paste Action
    icon = QIcon::fromTheme(QLatin1String("edit-paste"), Utils::Icons::PASTE.icon());
    tmpaction = new QAction(icon, tr("&Paste"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::PASTE);
    cmd->setDefaultKeySequence(QKeySequence::Paste);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Select All
    icon = QIcon::fromTheme(QLatin1String("edit-select-all"));
    tmpaction = new QAction(icon, tr("Select &All"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::SELECTALL);
    cmd->setDefaultKeySequence(QKeySequence::SelectAll);
    medit->addAction(cmd, Constants::G_EDIT_SELECTALL);
    tmpaction->setEnabled(false);

    // Goto Action
    icon = QIcon::fromTheme(QLatin1String("go-jump"));
    tmpaction = new QAction(icon, tr("&Go to Line..."), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::GOTO);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+L")));
    medit->addAction(cmd, Constants::G_EDIT_OTHER);
    tmpaction->setEnabled(false);

    // Zoom In Action
    icon = QIcon::hasThemeIcon("zoom-in") ? QIcon::fromTheme("zoom-in")
                                          : Utils::Icons::ZOOMIN_TOOLBAR.icon();
    tmpaction = new QAction(icon, tr("Zoom In"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::ZOOM_IN);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl++")));
    tmpaction->setEnabled(false);

    // Zoom Out Action
    icon = QIcon::hasThemeIcon("zoom-out") ? QIcon::fromTheme("zoom-out")
                                           : Utils::Icons::ZOOMOUT_TOOLBAR.icon();
    tmpaction = new QAction(icon, tr("Zoom Out"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::ZOOM_OUT);
    if (useMacShortcuts)
        cmd->setDefaultKeySequences({QKeySequence(tr("Ctrl+-")), QKeySequence(tr("Ctrl+Shift+-"))});
    else
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+-")));
    tmpaction->setEnabled(false);

    // Zoom Reset Action
    icon = QIcon::hasThemeIcon("zoom-original") ? QIcon::fromTheme("zoom-original")
                                                : Utils::Icons::EYE_OPEN_TOOLBAR.icon();
    tmpaction = new QAction(icon, tr("Original Size"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::ZOOM_RESET);
    cmd->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+0") : tr("Ctrl+0")));
    tmpaction->setEnabled(false);

    // Options Action
    mtools->appendGroup(Constants::G_TOOLS_OPTIONS);
    mtools->addSeparator(Constants::G_TOOLS_OPTIONS);

    m_optionsAction = new QAction(tr("&Options..."), this);
    m_optionsAction->setMenuRole(QAction::PreferencesRole);
    cmd = ActionManager::registerAction(m_optionsAction, Constants::OPTIONS);
    cmd->setDefaultKeySequence(QKeySequence::Preferences);
    mtools->addAction(cmd, Constants::G_TOOLS_OPTIONS);
    connect(m_optionsAction, &QAction::triggered, this, [] { ICore::showOptionsDialog(Id()); });

    mwindow->addSeparator(Constants::G_WINDOW_LIST);

    if (useMacShortcuts) {
        // Minimize Action
        QAction *minimizeAction = new QAction(tr("Minimize"), this);
        minimizeAction->setEnabled(false); // actual implementation in WindowSupport
        cmd = ActionManager::registerAction(minimizeAction, Constants::MINIMIZE_WINDOW);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+M")));
        mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);

        // Zoom Action
        QAction *zoomAction = new QAction(tr("Zoom"), this);
        zoomAction->setEnabled(false); // actual implementation in WindowSupport
        cmd = ActionManager::registerAction(zoomAction, Constants::ZOOM_WINDOW);
        mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
    }

    // Full Screen Action
    QAction *toggleFullScreenAction = new QAction(tr("Full Screen"), this);
    toggleFullScreenAction->setCheckable(!HostOsInfo::isMacHost());
    toggleFullScreenAction->setEnabled(false); // actual implementation in WindowSupport
    cmd = ActionManager::registerAction(toggleFullScreenAction, Constants::TOGGLE_FULLSCREEN);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Ctrl+Meta+F") : tr("Ctrl+Shift+F11")));
    if (HostOsInfo::isMacHost())
        cmd->setAttribute(Command::CA_UpdateText);
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);

    if (useMacShortcuts) {
        mwindow->addSeparator(Constants::G_WINDOW_SIZE);

        QAction *closeAction = new QAction(tr("Close Window"), this);
        closeAction->setEnabled(false);
        cmd = ActionManager::registerAction(closeAction, Constants::CLOSE_WINDOW);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Meta+W")));
        mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);

        mwindow->addSeparator(Constants::G_WINDOW_SIZE);
    }

    // Show Left Sidebar Action
    m_toggleLeftSideBarAction = new QAction(Utils::Icons::TOGGLE_LEFT_SIDEBAR.icon(),
                                            QCoreApplication::translate("Core", Constants::TR_SHOW_LEFT_SIDEBAR),
                                            this);
    m_toggleLeftSideBarAction->setCheckable(true);
    cmd = ActionManager::registerAction(m_toggleLeftSideBarAction, Constants::TOGGLE_LEFT_SIDEBAR);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Ctrl+0") : tr("Alt+0")));
    connect(m_toggleLeftSideBarAction, &QAction::triggered,
            this, [this](bool visible) { setSidebarVisible(visible, Side::Left); });
    ProxyAction *toggleLeftSideBarProxyAction =
            ProxyAction::proxyActionWithIcon(cmd->action(), Utils::Icons::TOGGLE_LEFT_SIDEBAR_TOOLBAR.icon());
    m_toggleLeftSideBarButton->setDefaultAction(toggleLeftSideBarProxyAction);
    mview->addAction(cmd, Constants::G_VIEW_VIEWS);
    m_toggleLeftSideBarAction->setEnabled(false);

    // Show Right Sidebar Action
    m_toggleRightSideBarAction = new QAction(Utils::Icons::TOGGLE_RIGHT_SIDEBAR.icon(),
                                             QCoreApplication::translate("Core", Constants::TR_SHOW_RIGHT_SIDEBAR),
                                             this);
    m_toggleRightSideBarAction->setCheckable(true);
    cmd = ActionManager::registerAction(m_toggleRightSideBarAction, Constants::TOGGLE_RIGHT_SIDEBAR);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Ctrl+Shift+0") : tr("Alt+Shift+0")));
    connect(m_toggleRightSideBarAction, &QAction::triggered,
            this, [this](bool visible) { setSidebarVisible(visible, Side::Right); });
    ProxyAction *toggleRightSideBarProxyAction =
            ProxyAction::proxyActionWithIcon(cmd->action(), Utils::Icons::TOGGLE_RIGHT_SIDEBAR_TOOLBAR.icon());
    m_toggleRightSideBarButton->setDefaultAction(toggleRightSideBarProxyAction);
    mview->addAction(cmd, Constants::G_VIEW_VIEWS);
    m_toggleRightSideBarButton->setEnabled(false);

    registerModeSelectorStyleActions();

    // Window->Views
    ActionContainer *mviews = ActionManager::createMenu(Constants::M_VIEW_VIEWS);
    mview->addMenu(mviews, Constants::G_VIEW_VIEWS);
    mviews->menu()->setTitle(tr("&Views"));

    // "Help" separators
    mhelp->addSeparator(Constants::G_HELP_SUPPORT);
    if (!HostOsInfo::isMacHost())
        mhelp->addSeparator(Constants::G_HELP_ABOUT);

    // About IDE Action
    icon = QIcon::fromTheme(QLatin1String("help-about"));
    if (HostOsInfo::isMacHost())
        tmpaction = new QAction(icon, tr("About &%1").arg(Constants::IDE_DISPLAY_NAME), this); // it's convention not to add dots to the about menu
    else
        tmpaction = new QAction(icon, tr("About &%1...").arg(Constants::IDE_DISPLAY_NAME), this);
    tmpaction->setMenuRole(QAction::AboutRole);
    cmd = ActionManager::registerAction(tmpaction, Constants::ABOUT_QTCREATOR);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
    connect(tmpaction, &QAction::triggered, this, &MainWindow::aboutQtCreator);

    //About Plugins Action
    tmpaction = new QAction(tr("About &Plugins..."), this);
    tmpaction->setMenuRole(QAction::ApplicationSpecificRole);
    cmd = ActionManager::registerAction(tmpaction, Constants::ABOUT_PLUGINS);
    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
    connect(tmpaction, &QAction::triggered, this, &MainWindow::aboutPlugins);
    // About Qt Action
//    tmpaction = new QAction(tr("About &Qt..."), this);
//    cmd = ActionManager::registerAction(tmpaction, Constants:: ABOUT_QT);
//    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
//    tmpaction->setEnabled(true);
//    connect(tmpaction, &QAction::triggered, qApp, &QApplication::aboutQt);
    // About sep
    if (!HostOsInfo::isMacHost()) { // doesn't have the "About" actions in the Help menu
        tmpaction = new QAction(this);
        tmpaction->setSeparator(true);
        cmd = ActionManager::registerAction(tmpaction, "QtCreator.Help.Sep.About");
        mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    }
}

void MainWindow::registerModeSelectorStyleActions()
{
    ActionContainer *mview = ActionManager::actionContainer(Constants::M_VIEW);

    // Cycle Mode Selector Styles
    m_cycleModeSelectorStyleAction = new QAction(tr("Cycle Mode Selector Styles"), this);
    ActionManager::registerAction(m_cycleModeSelectorStyleAction, Constants::CYCLE_MODE_SELECTOR_STYLE);
    connect(m_cycleModeSelectorStyleAction, &QAction::triggered, this, [this] {
        ModeManager::cycleModeStyle();
        updateModeSelectorStyleMenu();
    });

    // Mode Selector Styles
    ActionContainer *mmodeLayouts = ActionManager::createMenu(Constants::M_VIEW_MODESTYLES);
    mview->addMenu(mmodeLayouts, Constants::G_VIEW_VIEWS);
    QMenu *styleMenu = mmodeLayouts->menu();
    styleMenu->setTitle(tr("Mode Selector Style"));
    auto *stylesGroup = new QActionGroup(styleMenu);
    stylesGroup->setExclusive(true);

    m_setModeSelectorStyleIconsAndTextAction = stylesGroup->addAction(tr("Icons and Text"));
    connect(m_setModeSelectorStyleIconsAndTextAction, &QAction::triggered,
                                 [] { ModeManager::setModeStyle(ModeManager::Style::IconsAndText); });
    m_setModeSelectorStyleIconsAndTextAction->setCheckable(true);
    m_setModeSelectorStyleIconsOnlyAction = stylesGroup->addAction(tr("Icons Only"));
    connect(m_setModeSelectorStyleIconsOnlyAction, &QAction::triggered,
                                 [] { ModeManager::setModeStyle(ModeManager::Style::IconsOnly); });
    m_setModeSelectorStyleIconsOnlyAction->setCheckable(true);
    m_setModeSelectorStyleHiddenAction = stylesGroup->addAction(tr("Hidden"));
    connect(m_setModeSelectorStyleHiddenAction, &QAction::triggered,
                                 [] { ModeManager::setModeStyle(ModeManager::Style::Hidden); });
    m_setModeSelectorStyleHiddenAction->setCheckable(true);

    styleMenu->addActions(stylesGroup->actions());
}

void MainWindow::openFile()
{
    openFiles(EditorManager::getOpenFileNames(), ICore::SwitchMode);
}

static IDocumentFactory *findDocumentFactory(const QList<IDocumentFactory*> &fileFactories,
                                     const QFileInfo &fi)
{
    const QString typeName = Utils::mimeTypeForFile(fi).name();
    return Utils::findOrDefault(fileFactories, [typeName](IDocumentFactory *f) {
        return f->mimeTypes().contains(typeName);
    });
}

/*!
 * \internal
 * Either opens \a fileNames with editors or loads a project.
 *
 *  \a flags can be used to stop on first failure, indicate that a file name
 *  might include line numbers and/or switch mode to edit mode.
 *
 *  \a workingDirectory is used when files are opened by a remote client, since
 *  the file names are relative to the client working directory.
 *
 *  Returns the first opened document. Required to support the \c -block flag
 *  for client mode.
 *
 *  \sa IPlugin::remoteArguments()
 */
IDocument *MainWindow::openFiles(const QStringList &fileNames,
                                 ICore::OpenFilesFlags flags,
                                 const QString &workingDirectory)
{
    const QList<IDocumentFactory*> documentFactories = IDocumentFactory::allDocumentFactories();
    IDocument *res = nullptr;

    foreach (const QString &fileName, fileNames) {
        const QDir workingDir(workingDirectory.isEmpty() ? QDir::currentPath() : workingDirectory);
        const QFileInfo fi(workingDir, fileName);
        const QString absoluteFilePath = fi.absoluteFilePath();
        if (IDocumentFactory *documentFactory = findDocumentFactory(documentFactories, fi)) {
            IDocument *document = documentFactory->open(absoluteFilePath);
            if (!document) {
                if (flags & ICore::StopOnLoadFail)
                    return res;
            } else {
                if (!res)
                    res = document;
                if (flags & ICore::SwitchMode)
                    ModeManager::activateMode(Id(Constants::MODE_EDIT));
            }
        } else {
            QFlags<EditorManager::OpenEditorFlag> emFlags;
            if (flags & ICore::CanContainLineAndColumnNumbers)
                emFlags |=  EditorManager::CanContainLineAndColumnNumber;
            if (flags & ICore::SwitchSplitIfAlreadyVisible)
                emFlags |= EditorManager::SwitchSplitIfAlreadyVisible;
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
    EditorManagerPrivate::doEscapeKeyFocusMoveMagic();
}

void MainWindow::exit()
{
    // this function is most likely called from a user action
    // that is from an event handler of an object
    // since on close we are going to delete everything
    // so to prevent the deleting of that object we
    // just append it
    QTimer::singleShot(0, this,  &QWidget::close);
}

void MainWindow::openFileWith()
{
    foreach (const QString &fileName, EditorManager::getOpenFileNames()) {
        bool isExternal;
        const Id editorId = EditorManagerPrivate::getOpenWithEditorId(fileName, &isExternal);
        if (!editorId.isValid())
            continue;
        if (isExternal)
            EditorManager::openExternalEditor(fileName, editorId);
        else
            EditorManagerPrivate::openEditorWith(fileName, editorId);
    }
}

IContext *MainWindow::contextObject(QWidget *widget) const
{
    const auto it = m_contextWidgets.find(widget);
    return it == m_contextWidgets.end() ? nullptr : it->second;
}

void MainWindow::addContextObject(IContext *context)
{
    if (!context)
        return;
    QWidget *widget = context->widget();
    if (m_contextWidgets.find(widget) != m_contextWidgets.end())
        return;

    m_contextWidgets.insert(std::make_pair(widget, context));
    connect(context, &QObject::destroyed, this, [this, context] { removeContextObject(context); });
}

void MainWindow::removeContextObject(IContext *context)
{
    if (!context)
        return;

    disconnect(context, &QObject::destroyed, this, nullptr);

    const auto it = std::find_if(m_contextWidgets.cbegin(),
                                 m_contextWidgets.cend(),
                                 [context](const std::pair<QWidget *, IContext *> &v) {
                                     return v.second == context;
                                 });
    if (it == m_contextWidgets.cend())
        return;

    m_contextWidgets.erase(it);
    if (m_activeContext.removeAll(context) > 0)
        updateContextObject(m_activeContext);
}

void MainWindow::updateFocusWidget(QWidget *old, QWidget *now)
{
    Q_UNUSED(old)

    // Prevent changing the context object just because the menu or a menu item is activated
    if (qobject_cast<QMenuBar*>(now) || qobject_cast<QMenu*>(now))
        return;

    QList<IContext *> newContext;
    if (QWidget *p = QApplication::focusWidget()) {
        IContext *context = nullptr;
        while (p) {
            context = contextObject(p);
            if (context)
                newContext.append(context);
            p = p->parentWidget();
        }
    }

    // ignore toplevels that define no context, like popups without parent
    if (!newContext.isEmpty() || QApplication::focusWidget() == focusWidget())
        updateContextObject(newContext);
}

void MainWindow::updateContextObject(const QList<IContext *> &context)
{
    emit m_coreImpl->contextAboutToChange(context);
    m_activeContext = context;
    updateContext();
    if (debugMainWindow) {
        qDebug() << "new context objects =" << context;
        foreach (IContext *c, context)
            qDebug() << (c ? c->widget() : nullptr) << (c ? c->widget()->metaObject()->className() : nullptr);
    }
}

void MainWindow::aboutToShutdown()
{
    disconnect(qApp, &QApplication::focusChanged, this, &MainWindow::updateFocusWidget);
    m_activeContext.clear();
    hide();
}

static const char settingsGroup[] = "MainWindow";
static const char colorKey[] = "Color";
static const char windowGeometryKey[] = "WindowGeometry";
static const char windowStateKey[] = "WindowState";
static const char modeSelectorLayoutKey[] = "ModeSelectorLayout";

void MainWindow::readSettings()
{
    QSettings *settings = PluginManager::settings();
    settings->beginGroup(QLatin1String(settingsGroup));

    if (m_overrideColor.isValid()) {
        StyleHelper::setBaseColor(m_overrideColor);
        // Get adapted base color.
        m_overrideColor = StyleHelper::baseColor();
    } else {
        StyleHelper::setBaseColor(settings->value(QLatin1String(colorKey),
                                  QColor(StyleHelper::DEFAULT_BASE_COLOR)).value<QColor>());
    }

    {
        ModeManager::Style modeStyle =
                ModeManager::Style(settings->value(modeSelectorLayoutKey, int(ModeManager::Style::IconsAndText)).toInt());

        // Migrate legacy setting from Qt Creator 4.6 and earlier
        static const char modeSelectorVisibleKey[] = "ModeSelectorVisible";
        if (!settings->contains(modeSelectorLayoutKey) && settings->contains(modeSelectorVisibleKey)) {
            bool visible = settings->value(modeSelectorVisibleKey, true).toBool();
            modeStyle = visible ? ModeManager::Style::IconsAndText : ModeManager::Style::Hidden;
        }

        ModeManager::setModeStyle(modeStyle);
        updateModeSelectorStyleMenu();
    }

    settings->endGroup();

    EditorManagerPrivate::readSettings();
    m_leftNavigationWidget->restoreSettings(settings);
    m_rightNavigationWidget->restoreSettings(settings);
    m_rightPaneWidget->readSettings(settings);
}

void MainWindow::saveSettings()
{
    QSettings *settings = PluginManager::settings();
    settings->beginGroup(QLatin1String(settingsGroup));

    if (!(m_overrideColor.isValid() && StyleHelper::baseColor() == m_overrideColor))
        settings->setValue(QLatin1String(colorKey), StyleHelper::requestedBaseColor());

    settings->endGroup();

    DocumentManager::saveSettings();
    ActionManager::saveSettings();
    EditorManagerPrivate::saveSettings();
    m_leftNavigationWidget->saveSettings(settings);
    m_rightNavigationWidget->saveSettings(settings);
}

void MainWindow::saveWindowSettings()
{
    QSettings *settings = PluginManager::settings();
    settings->beginGroup(QLatin1String(settingsGroup));

    // On OS X applications usually do not restore their full screen state.
    // To be able to restore the correct non-full screen geometry, we have to put
    // the window out of full screen before saving the geometry.
    // Works around QTBUG-45241
    if (Utils::HostOsInfo::isMacHost() && isFullScreen())
        setWindowState(windowState() & ~Qt::WindowFullScreen);
    settings->setValue(QLatin1String(windowGeometryKey), saveGeometry());
    settings->setValue(QLatin1String(windowStateKey), saveState());
    settings->setValue(modeSelectorLayoutKey, int(ModeManager::modeStyle()));

    settings->endGroup();
}

void MainWindow::updateModeSelectorStyleMenu()
{
    switch (ModeManager::modeStyle()) {
    case ModeManager::Style::IconsAndText:
        m_setModeSelectorStyleIconsAndTextAction->setChecked(true);
        break;
    case ModeManager::Style::IconsOnly:
        m_setModeSelectorStyleIconsOnlyAction->setChecked(true);
        break;
    case ModeManager::Style::Hidden:
        m_setModeSelectorStyleHiddenAction->setChecked(true);
        break;
    }
}

void MainWindow::updateAdditionalContexts(const Context &remove, const Context &add,
                                          ICore::ContextPriority priority)
{
    foreach (const Id id, remove) {
        if (!id.isValid())
            continue;
        int index = m_lowPrioAdditionalContexts.indexOf(id);
        if (index != -1)
            m_lowPrioAdditionalContexts.removeAt(index);
        index = m_highPrioAdditionalContexts.indexOf(id);
        if (index != -1)
            m_highPrioAdditionalContexts.removeAt(index);
    }

    foreach (const Id id, add) {
        if (!id.isValid())
            continue;
        Context &cref = (priority == ICore::ContextPriority::High ? m_highPrioAdditionalContexts
                                                                  : m_lowPrioAdditionalContexts);
        if (!cref.contains(id))
            cref.prepend(id);
    }

    updateContext();
}

void MainWindow::updateContext()
{
    Context contexts = m_highPrioAdditionalContexts;

    foreach (IContext *context, m_activeContext)
        contexts.add(context->context());

    contexts.add(m_lowPrioAdditionalContexts);

    Context uniquecontexts;
    for (const Id &id : qAsConst(contexts)) {
        if (!uniquecontexts.contains(id))
            uniquecontexts.add(id);
    }

    ActionManager::setContext(uniquecontexts);
    emit m_coreImpl->contextChanged(uniquecontexts);
}

void MainWindow::aboutToShowRecentFiles()
{
    ActionContainer *aci = ActionManager::actionContainer(Constants::M_FILE_RECENTFILES);
    QMenu *menu = aci->menu();
    menu->clear();

    const QList<DocumentManager::RecentFile> recentFiles = DocumentManager::recentFiles();
    for (int i = 0; i < recentFiles.count(); ++i) {
        const DocumentManager::RecentFile file = recentFiles[i];

        const QString filePath
                = Utils::quoteAmpersands(QDir::toNativeSeparators(withTildeHomePath(file.first)));
        const QString actionText = ActionManager::withNumberAccelerator(filePath, i + 1);
        QAction *action = menu->addAction(actionText);
        connect(action, &QAction::triggered, this, [file] {
            EditorManager::openEditor(file.first, file.second);
        });
    }

    bool hasRecentFiles = !recentFiles.isEmpty();
    menu->setEnabled(hasRecentFiles);

    // add the Clear Menu item
    if (hasRecentFiles) {
        menu->addSeparator();
        QAction *action = menu->addAction(QCoreApplication::translate(
                                                     "Core", Constants::TR_CLEAR_MENU));
        connect(action, &QAction::triggered,
                DocumentManager::instance(), &DocumentManager::clearRecentFiles);
    }
}

void MainWindow::aboutQtCreator()
{
    if (!m_versionDialog) {
        m_versionDialog = new VersionDialog(this);
        connect(m_versionDialog, &QDialog::finished,
                this, &MainWindow::destroyVersionDialog);
        ICore::registerWindow(m_versionDialog, Context("Core.VersionDialog"));
        m_versionDialog->show();
    } else {
        ICore::raiseWindow(m_versionDialog);
    }
}

void MainWindow::destroyVersionDialog()
{
    if (m_versionDialog) {
        m_versionDialog->deleteLater();
        m_versionDialog = nullptr;
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

void MainWindow::restoreWindowState()
{
    QSettings *settings = PluginManager::settings();
    settings->beginGroup(QLatin1String(settingsGroup));
    if (!restoreGeometry(settings->value(QLatin1String(windowGeometryKey)).toByteArray()))
        resize(1260, 700); // size without window decoration
    restoreState(settings->value(QLatin1String(windowStateKey)).toByteArray());
    settings->endGroup();
    show();
    StatusBarManager::restoreSettings();
}

} // namespace Internal
} // namespace Core
