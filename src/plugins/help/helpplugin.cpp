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

#include "helpplugin.h"

#include "bookmarkmanager.h"
#include "centralwidget.h"
#include "contentwindow.h"
#include "docsettingspage.h"
#include "externalhelpwindow.h"
#include "filtersettingspage.h"
#include "generalsettingspage.h"
#include "helpconstants.h"
#include "helpfindsupport.h"
#include "helpindexfilter.h"
#include "helpmode.h"
#include "helpviewer.h"
#include "indexwindow.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"
#include "openpagesmodel.h"
#include "remotehelpfilter.h"
#include "searchwidget.h"

#include <app/app_version.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/sidebar.h>
#include <extensionsystem/pluginmanager.h>
#include <find/findplugin.h>
#include <texteditor/texteditorconstants.h>
#include <utils/hostosinfo.h>
#include <utils/styledbar.h>

#include <QDir>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QTimer>
#include <QTranslator>
#include <qplugin.h>
#include <QRegExp>

#include <QAction>
#include <QComboBox>
#include <QDesktopServices>
#include <QMenu>
#include <QShortcut>
#include <QStackedLayout>
#include <QSplitter>

#include <QHelpEngine>

#if !defined(QT_NO_WEBKIT)
#include <QWebElement>
#include <QWebElementCollection>
#include <QWebFrame>
#include <QWebHistory>
#endif

using namespace Help::Internal;

const char SB_INDEX[] = QT_TRANSLATE_NOOP("Help::Internal::HelpPlugin", "Index");
const char SB_CONTENTS[] = QT_TRANSLATE_NOOP("Help::Internal::HelpPlugin", "Contents");
const char SB_BOOKMARKS[] = QT_TRANSLATE_NOOP("Help::Internal::HelpPlugin", "Bookmarks");
const char SB_SEARCH[] = QT_TRANSLATE_NOOP("Help::Internal::HelpPlugin", "Search");

const char SB_OPENPAGES[] = "OpenPages";

#define IMAGEPATH ":/help/images/"
#if defined(Q_OS_MAC)
#   define DOCPATH "/../Resources/doc/"
#else
#   define DOCPATH "/../share/doc/qtcreator/"
#endif

using namespace Core;

static QToolButton *toolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    button->setPopupMode(QToolButton::DelayedPopup);
    return button;
}

HelpPlugin::HelpPlugin()
    : m_mode(0),
    m_centralWidget(0),
    m_rightPaneSideBarWidget(0),
    m_helpViewerForSideBar(0),
    m_contentItem(0),
    m_indexItem(0),
    m_searchItem(0),
    m_bookmarkItem(0),
    m_sideBar(0),
    m_firstModeChange(true),
    m_helpManager(0),
    m_openPagesManager(0),
    m_oldMode(0),
    m_connectWindow(true),
    m_externalWindow(0),
    m_backMenu(0),
    m_nextMenu(0),
    m_isSidebarVisible(true)
{
}

HelpPlugin::~HelpPlugin()
{
    delete m_centralWidget;
    delete m_openPagesManager;
    delete m_rightPaneSideBarWidget;

    delete m_helpManager;
}

bool HelpPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error)
    Core::Context globalcontext(Core::Constants::C_GLOBAL);
    Core::Context modecontext(Constants::C_MODE_HELP);

    const QString &locale = Core::ICore::userInterfaceLanguage();
    if (!locale.isEmpty()) {
        QTranslator *qtr = new QTranslator(this);
        QTranslator *qhelptr = new QTranslator(this);
        const QString &creatorTrPath = Core::ICore::resourcePath()
            + QLatin1String("/translations");
        const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString &trFile = QLatin1String("assistant_") + locale;
        const QString &helpTrFile = QLatin1String("qt_help_") + locale;
        if (qtr->load(trFile, qtTrPath) || qtr->load(trFile, creatorTrPath))
            qApp->installTranslator(qtr);
        if (qhelptr->load(helpTrFile, qtTrPath) || qhelptr->load(helpTrFile, creatorTrPath))
            qApp->installTranslator(qhelptr);
    }

    m_helpManager = new LocalHelpManager(this);
    m_openPagesManager = new OpenPagesManager(this);
    addAutoReleasedObject(m_docSettingsPage = new DocSettingsPage());
    addAutoReleasedObject(m_filterSettingsPage = new FilterSettingsPage());
    addAutoReleasedObject(m_generalSettingsPage = new GeneralSettingsPage());

    connect(m_generalSettingsPage, SIGNAL(fontChanged()), this,
        SLOT(fontChanged()));
    connect(m_generalSettingsPage, SIGNAL(contextHelpOptionChanged()), this,
        SLOT(contextHelpOptionChanged()));
    connect(m_generalSettingsPage, SIGNAL(returnOnCloseChanged()), this,
        SLOT(updateCloseButton()));
    connect(Core::HelpManager::instance(), SIGNAL(helpRequested(QUrl)), this,
        SLOT(handleHelpRequest(QUrl)));

    connect(m_filterSettingsPage, SIGNAL(filtersChanged()), this,
        SLOT(setupHelpEngineIfNeeded()));
    connect(Core::HelpManager::instance(), SIGNAL(documentationChanged()), this,
        SLOT(setupHelpEngineIfNeeded()));
    connect(Core::HelpManager::instance(), SIGNAL(collectionFileChanged()), this,
        SLOT(setupHelpEngineIfNeeded()));
    connect(Core::HelpManager::instance(), SIGNAL(setupFinished()), this,
            SLOT(unregisterOldQtCreatorDocumentation()));

    m_splitter = new Core::MiniSplitter;
    m_centralWidget = new Help::Internal::CentralWidget();
    connect(m_centralWidget, SIGNAL(sourceChanged(QUrl)), this,
        SLOT(updateSideBarSource(QUrl)));
    connect(m_centralWidget, SIGNAL(openFindToolBar()), this,
        SLOT(openFindToolBar()));

    // Add Home, Previous and Next actions (used in the toolbar)
    QAction *action = new QAction(QIcon(QLatin1String(IMAGEPATH "home.png")),
        tr("Home"), this);
    Core::ActionManager::registerAction(action, "Help.Home", globalcontext);
    connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(home()));

    action = new QAction(QIcon(QLatin1String(IMAGEPATH "previous.png")),
        tr("Previous Page"), this);
    Core::Command *cmd = Core::ActionManager::registerAction(action, Core::Id("Help.Previous"), modecontext);
    cmd->setDefaultKeySequence(QKeySequence::Back);
    action->setEnabled(m_centralWidget->isBackwardAvailable());
    connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(backward()));
    connect(m_centralWidget, SIGNAL(backwardAvailable(bool)), action,
        SLOT(setEnabled(bool)));

    action = new QAction(QIcon(QLatin1String(IMAGEPATH "next.png")), tr("Next Page"),
        this);
    cmd = Core::ActionManager::registerAction(action, Core::Id("Help.Next"), modecontext);
    cmd->setDefaultKeySequence(QKeySequence::Forward);
    action->setEnabled(m_centralWidget->isForwardAvailable());
    connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(forward()));
    connect(m_centralWidget, SIGNAL(forwardAvailable(bool)), action,
        SLOT(setEnabled(bool)));

    action = new QAction(QIcon(QLatin1String(IMAGEPATH "bookmark.png")),
        tr("Add Bookmark"), this);
    cmd = Core::ActionManager::registerAction(action, Core::Id("Help.AddBookmark"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+M") : tr("Ctrl+M")));
    connect(action, SIGNAL(triggered()), this, SLOT(addBookmark()));

    // Add Contents, Index, and Context menu items and a separator to the Help menu
    action = new QAction(QIcon::fromTheme(QLatin1String("help-contents")),
        tr(SB_CONTENTS), this);
    cmd = Core::ActionManager::registerAction(action, Core::Id("Help.Contents"), globalcontext);
    Core::ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, SIGNAL(triggered()), this, SLOT(activateContents()));

    action = new QAction(tr(SB_INDEX), this);
    cmd = Core::ActionManager::registerAction(action, Core::Id("Help.Index"), globalcontext);
    Core::ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, SIGNAL(triggered()), this, SLOT(activateIndex()));

    action = new QAction(tr("Context Help"), this);
    cmd = Core::ActionManager::registerAction(action, Core::Id("Help.Context"), globalcontext);
    Core::ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F1));
    connect(action, SIGNAL(triggered()), this, SLOT(activateContext()));

    if (!Utils::HostOsInfo::isMacHost()) {
        action = new QAction(this);
        action->setSeparator(true);
        cmd = Core::ActionManager::registerAction(action, Core::Id("Help.Separator"), globalcontext);
        Core::ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    }

    action = new QAction(tr("Technical Support"), this);
    cmd = Core::ActionManager::registerAction(action, Core::Id("Help.TechSupport"), globalcontext);
    Core::ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, SIGNAL(triggered()), this, SLOT(slotOpenSupportPage()));

    action = new QAction(tr("Report Bug..."), this);
    cmd = Core::ActionManager::registerAction(action, Core::Id("Help.ReportBug"), globalcontext);
    Core::ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, SIGNAL(triggered()), this, SLOT(slotReportBug()));

    if (!Utils::HostOsInfo::isMacHost()) {
        action = new QAction(this);
        action->setSeparator(true);
        cmd = Core::ActionManager::registerAction(action, Core::Id("Help.Separator2"), globalcontext);
        Core::ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    }

    action = new QAction(this);
    Core::ActionManager::registerAction(action, Core::Constants::PRINT, modecontext);
    connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(print()));

    action = new QAction(this);
    cmd = Core::ActionManager::registerAction(action, Core::Constants::COPY, modecontext);
    connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(copy()));
    action->setText(cmd->action()->text());
    action->setIcon(cmd->action()->icon());

    if (Core::ActionContainer *advancedMenu = Core::ActionManager::actionContainer(Core::Constants::M_EDIT_ADVANCED)) {
        // reuse TextEditor constants to avoid a second pair of menu actions
        action = new QAction(tr("Increase Font Size"), this);
        cmd = Core::ActionManager::registerAction(action, TextEditor::Constants::INCREASE_FONT_SIZE,
            modecontext);
        connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(zoomIn()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);

        action = new QAction(tr("Decrease Font Size"), this);
        cmd = Core::ActionManager::registerAction(action, TextEditor::Constants::DECREASE_FONT_SIZE,
            modecontext);
        connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(zoomOut()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);

        action = new QAction(tr("Reset Font Size"), this);
        cmd = Core::ActionManager::registerAction(action, TextEditor::Constants::RESET_FONT_SIZE,
            modecontext);
        connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(resetZoom()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);
    }

    if (Core::ActionContainer *windowMenu = Core::ActionManager::actionContainer(Core::Constants::M_WINDOW)) {
        // reuse EditorManager constants to avoid a second pair of menu actions
        action = new QAction(QApplication::translate("EditorManager",
            "Next Open Document in History"), this);
        Core::Command *ctrlTab = Core::ActionManager::registerAction(action, Core::Constants::GOTOPREVINHISTORY,
            modecontext);   // Goto Previous In History Action
        windowMenu->addAction(ctrlTab, Core::Constants::G_WINDOW_NAVIGATE);
        connect(action, SIGNAL(triggered()), &OpenPagesManager::instance(),
            SLOT(gotoPreviousPage()));

        action = new QAction(QApplication::translate("EditorManager",
            "Previous Open Document in History"), this);
        Core::Command *ctrlShiftTab = Core::ActionManager::registerAction(action, Core::Constants::GOTONEXTINHISTORY,
            modecontext);   // Goto Next In History Action
        windowMenu->addAction(ctrlShiftTab, Core::Constants::G_WINDOW_NAVIGATE);
        connect(action, SIGNAL(triggered()), &OpenPagesManager::instance(),
            SLOT(gotoNextPage()));
    }

    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(m_centralWidget);
    agg->add(new HelpFindSupport(m_centralWidget));

    QWidget *toolBarWidget = new QWidget;
    QHBoxLayout *toolBarLayout = new QHBoxLayout(toolBarWidget);
    toolBarLayout->setMargin(0);
    toolBarLayout->setSpacing(0);
    toolBarLayout->addWidget(m_externalHelpBar = createIconToolBar(true));
    toolBarLayout->addWidget(m_internalHelpBar = createIconToolBar(false));
    toolBarLayout->addWidget(createWidgetToolBar());

    QWidget *mainWidget = new QWidget;
    m_splitter->addWidget(mainWidget);
    QVBoxLayout *mainWidgetLayout = new QVBoxLayout(mainWidget);
    mainWidgetLayout->setMargin(0);
    mainWidgetLayout->setSpacing(0);
    mainWidgetLayout->addWidget(toolBarWidget);
    mainWidgetLayout->addWidget(m_centralWidget);

    if (QLayout *layout = m_centralWidget->layout()) {
        layout->setSpacing(0);
        Core::FindToolBarPlaceHolder *fth = new Core::FindToolBarPlaceHolder(m_centralWidget);
        fth->setObjectName(QLatin1String("HelpFindToolBarPlaceHolder"));
        layout->addWidget(fth);
    }

    HelpIndexFilter *helpIndexFilter = new HelpIndexFilter();
    addAutoReleasedObject(helpIndexFilter);
    connect(helpIndexFilter, SIGNAL(linkActivated(QUrl)), this,
        SLOT(switchToHelpMode(QUrl)));

    RemoteHelpFilter *remoteHelpFilter = new RemoteHelpFilter();
    addAutoReleasedObject(remoteHelpFilter);
    connect(remoteHelpFilter, SIGNAL(linkActivated(QUrl)), this,
        SLOT(switchToHelpMode(QUrl)));

    QDesktopServices::setUrlHandler(QLatin1String("qthelp"), this, "handleHelpRequest");
    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*,
        Core::IMode*)), this, SLOT(modeChanged(Core::IMode*,Core::IMode*)));

    m_externalWindow = new ExternalHelpWindow;
    m_mode = new HelpMode;
    if (contextHelpOption() == Help::Constants::ExternalHelpAlways) {
        m_mode->setWidget(new QWidget);
        m_mode->setEnabled(false);
        m_externalHelpBar->setVisible(true);
        m_externalWindow->setCentralWidget(m_splitter);
        QTimer::singleShot(0, this, SLOT(showExternalWindow()));
    } else {
        m_mode->setWidget(m_splitter);
        m_internalHelpBar->setVisible(true);
    }
    addAutoReleasedObject(m_mode);

    return true;
}

void HelpPlugin::extensionsInitialized()
{
    QStringList filesToRegister;
    // Explicitly register qml.qch if located in creator directory. This is only
    // needed for the creator-qml package, were we want to ship the documentation
    // without a qt development version. TODO: is this still really needed, remove
    const QString &appPath = QCoreApplication::applicationDirPath();
    filesToRegister.append(QDir::cleanPath(QDir::cleanPath(appPath
        + QLatin1String(DOCPATH "qml.qch"))));

    // we might need to register creators inbuild help
    filesToRegister.append(QDir::cleanPath(appPath
        + QLatin1String(DOCPATH "qtcreator.qch")));
    Core::HelpManager::instance()->registerDocumentation(filesToRegister);
}

ExtensionSystem::IPlugin::ShutdownFlag HelpPlugin::aboutToShutdown()
{
    if (m_sideBar) {
        QSettings *settings = Core::ICore::settings();
        m_sideBar->saveSettings(settings, QLatin1String("HelpSideBar"));
        // keep a boolean value to avoid to modify the sidebar class, at least some qml stuff
        // depends on the always visible property of the sidebar...
        settings->setValue(QLatin1String("HelpSideBar/") + QLatin1String("Visible"), m_isSidebarVisible);
    }

    if (m_externalWindow) {
        delete m_externalWindow;
        m_centralWidget = 0; // Running the external window will take down the central widget as well, cause
            // calling m_externalWindow->setCentralWidget(m_centralWidget) will pass ownership to the window.
    }

    return SynchronousShutdown;
}

void HelpPlugin::unregisterOldQtCreatorDocumentation()
{
    const QString &nsInternal = QString::fromLatin1("com.nokia.qtcreator.%1%2%3")
        .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR).arg(IDE_VERSION_RELEASE);

    Core::HelpManager *helpManager = Core::HelpManager::instance();
    QStringList documentationToUnregister;
    foreach (const QString &ns, helpManager->registeredNamespaces()) {
        if (ns.startsWith(QLatin1String("com.nokia.qtcreator."))
                && ns != nsInternal) {
            documentationToUnregister << ns;
        }
    }
    if (!documentationToUnregister.isEmpty())
        helpManager->unregisterDocumentation(documentationToUnregister);
}

void HelpPlugin::setupUi()
{
    // side bar widgets and shortcuts
    Core::Context modecontext(Constants::C_MODE_HELP);

    IndexWindow *indexWindow = new IndexWindow();
    indexWindow->setWindowTitle(tr(SB_INDEX));
    m_indexItem = new Core::SideBarItem(indexWindow, QLatin1String(SB_INDEX));

    connect(indexWindow, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));
    connect(indexWindow, SIGNAL(linksActivated(QMap<QString,QUrl>,QString)),
        m_centralWidget, SLOT(showTopicChooser(QMap<QString,QUrl>,QString)));

    QMap<QString, Core::Command*> shortcutMap;
    QShortcut *shortcut = new QShortcut(m_splitter);
    shortcut->setWhatsThis(tr("Activate Index in Help mode"));
    Core::Command* cmd = Core::ActionManager::registerShortcut(shortcut,
        Core::Id("Help.IndexShortcut"), modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+I") : tr("Ctrl+Shift+I")));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateIndex()));
    shortcutMap.insert(QLatin1String(SB_INDEX), cmd);

    ContentWindow *contentWindow = new ContentWindow();
    contentWindow->setWindowTitle(tr(SB_CONTENTS));
    m_contentItem = new Core::SideBarItem(contentWindow, QLatin1String(SB_CONTENTS));
    connect(contentWindow, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));

    shortcut = new QShortcut(m_splitter);
    shortcut->setWhatsThis(tr("Activate Contents in Help mode"));
    cmd = Core::ActionManager::registerShortcut(shortcut, Core::Id("Help.ContentsShortcut"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+Shift+C") : tr("Ctrl+Shift+C")));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateContents()));
    shortcutMap.insert(QLatin1String(SB_CONTENTS), cmd);

    SearchWidget *searchWidget = new SearchWidget();
    searchWidget->setWindowTitle(tr(SB_SEARCH));
    m_searchItem = new Core::SideBarItem(searchWidget, QLatin1String(SB_SEARCH));
    connect(searchWidget, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSourceFromSearch(QUrl)));

     shortcut = new QShortcut(m_splitter);
     shortcut->setWhatsThis(tr("Activate Search in Help mode"));
     cmd = Core::ActionManager::registerShortcut(shortcut, Core::Id("Help.SearchShortcut"),
         modecontext);
     cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+/") : tr("Ctrl+Shift+/")));
     connect(shortcut, SIGNAL(activated()), this, SLOT(activateSearch()));
     shortcutMap.insert(QLatin1String(SB_SEARCH), cmd);

    BookmarkManager *manager = &LocalHelpManager::bookmarkManager();
    BookmarkWidget *bookmarkWidget = new BookmarkWidget(manager, 0, false);
    bookmarkWidget->setWindowTitle(tr(SB_BOOKMARKS));
    m_bookmarkItem = new Core::SideBarItem(bookmarkWidget, QLatin1String(SB_BOOKMARKS));
    connect(bookmarkWidget, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));
    connect(bookmarkWidget, SIGNAL(createPage(QUrl,bool)), &OpenPagesManager::instance(),
            SLOT(createPage(QUrl,bool)));

     shortcut = new QShortcut(m_splitter);
     shortcut->setWhatsThis(tr("Activate Bookmarks in Help mode"));
     cmd = Core::ActionManager::registerShortcut(shortcut, Core::Id("Help.BookmarkShortcut"),
         modecontext);
     cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+B") : tr("Ctrl+Shift+B")));
     connect(shortcut, SIGNAL(activated()), this, SLOT(activateBookmarks()));
     shortcutMap.insert(QLatin1String(SB_BOOKMARKS), cmd);

    QWidget *openPagesWidget = OpenPagesManager::instance().openPagesWidget();
    openPagesWidget->setWindowTitle(tr("Open Pages"));
    m_openPagesItem = new Core::SideBarItem(openPagesWidget, QLatin1String(SB_OPENPAGES));

    shortcut = new QShortcut(m_splitter);
    shortcut->setWhatsThis(tr("Activate Open Pages in Help mode"));
    cmd = Core::ActionManager::registerShortcut(shortcut, Core::Id("Help.PagesShortcut"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+O") : tr("Ctrl+Shift+O")));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateOpenPages()));
    shortcutMap.insert(QLatin1String(SB_OPENPAGES), cmd);

    QList<Core::SideBarItem*> itemList;
    itemList << m_contentItem << m_indexItem << m_searchItem << m_bookmarkItem
        << m_openPagesItem;
    m_sideBar = new Core::SideBar(itemList, QList<Core::SideBarItem*>()
        << m_contentItem << m_openPagesItem);
    m_sideBar->setCloseWhenEmpty(true);
    m_sideBar->setShortcutMap(shortcutMap);
    connect(m_sideBar, SIGNAL(sideBarClosed()), this, SLOT(onSideBarVisibilityChanged()));

    m_splitter->setOpaqueResize(false);
    m_splitter->insertWidget(0, m_sideBar);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_sideBar->readSettings(Core::ICore::settings(), QLatin1String("HelpSideBar"));
    m_splitter->setSizes(QList<int>() << m_sideBar->size().width() << 300);

    m_toggleSideBarAction = new QAction(QIcon(QLatin1String(Core::Constants::ICON_TOGGLE_SIDEBAR)),
        tr("Show Sidebar"), this);
    m_toggleSideBarAction->setCheckable(true);
    connect(m_toggleSideBarAction, SIGNAL(triggered(bool)), this, SLOT(showHideSidebar()));
    cmd = Core::ActionManager::registerAction(m_toggleSideBarAction, Core::Constants::TOGGLE_SIDEBAR, modecontext);
}

void HelpPlugin::resetFilter()
{
    const QString &filterInternal = QString::fromLatin1("Qt Creator %1.%2.%3")
        .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR).arg(IDE_VERSION_RELEASE);
    QRegExp filterRegExp(QLatin1String("Qt Creator \\d*\\.\\d*\\.\\d*"));

    QHelpEngineCore *engine = &LocalHelpManager::helpEngine();
    const QStringList &filters = engine->customFilters();
    foreach (const QString &filter, filters) {
        if (filterRegExp.exactMatch(filter) && filter != filterInternal)
            engine->removeCustomFilter(filter);
    }

    // we added a filter at some point, remove previously added filter
    if (engine->customValue(Help::Constants::WeAddedFilterKey).toInt() == 1) {
        const QString &filter =
            engine->customValue(Help::Constants::PreviousFilterNameKey).toString();
        if (!filter.isEmpty())
            engine->removeCustomFilter(filter);
    }

    // potentially remove a filter with new name
    const QString filterName = tr("Unfiltered");
    engine->removeCustomFilter(filterName);
    engine->addCustomFilter(filterName, QStringList());
    engine->setCustomValue(Help::Constants::WeAddedFilterKey, 1);
    engine->setCustomValue(Help::Constants::PreviousFilterNameKey, filterName);
    engine->setCurrentFilter(filterName);

    updateFilterComboBox();
    connect(engine, SIGNAL(setupFinished()), this, SLOT(updateFilterComboBox()));
}

void HelpPlugin::createRightPaneContextViewer()
{
    if (m_rightPaneSideBarWidget)
        return;

    Utils::StyledBar *toolBar = new Utils::StyledBar();

    QAction *switchToHelp = new QAction(tr("Go to Help Mode"), toolBar);
    connect(switchToHelp, SIGNAL(triggered()), this, SLOT(switchToHelpMode()));
    QAction *back = new QAction(QIcon(QLatin1String(IMAGEPATH "previous.png")),
        tr("Previous"), toolBar);
    QAction *next = new QAction(QIcon(QLatin1String(IMAGEPATH "next.png")),
        tr("Next"), toolBar);
    QAction *close = new QAction(QIcon(QLatin1String(Core::Constants::ICON_CLOSE_DOCUMENT)),
        QLatin1String(""), toolBar);
    connect(close, SIGNAL(triggered()), this, SLOT(slotHideRightPane()));

    setupNavigationMenus(back, next, toolBar);

    QHBoxLayout *layout = new QHBoxLayout(toolBar);
    layout->setSpacing(0);
    layout->setMargin(0);

    layout->addWidget(toolButton(switchToHelp));
    layout->addWidget(toolButton(back));
    layout->addWidget(toolButton(next));
    layout->addStretch();
    layout->addWidget(toolButton(close));

    m_rightPaneSideBarWidget = new QWidget;
    m_helpViewerForSideBar = new HelpViewer(qreal(0.0));
    connect(m_helpViewerForSideBar, SIGNAL(openFindToolBar()), this,
        SLOT(openFindToolBar()));
#if !defined(QT_NO_WEBKIT)
    m_helpViewerForSideBar->pageAction(QWebPage::OpenLinkInNewWindow)->setVisible(false);
#endif

    QVBoxLayout *rightPaneLayout = new QVBoxLayout(m_rightPaneSideBarWidget);
    rightPaneLayout->setMargin(0);
    rightPaneLayout->setSpacing(0);
    rightPaneLayout->addWidget(toolBar);
    rightPaneLayout->addWidget(m_helpViewerForSideBar);
    Core::FindToolBarPlaceHolder *fth = new Core::FindToolBarPlaceHolder(m_rightPaneSideBarWidget);
    fth->setObjectName(QLatin1String("HelpRightPaneFindToolBarPlaceHolder"));
    rightPaneLayout->addWidget(fth);
    m_rightPaneSideBarWidget->setFocusProxy(m_helpViewerForSideBar);

    Aggregation::Aggregate *agg = new Aggregation::Aggregate();
    agg->add(m_helpViewerForSideBar);
    agg->add(new HelpViewerFindSupport(m_helpViewerForSideBar));

    Core::Context context(Constants::C_HELP_SIDEBAR);
    Core::IContext *icontext = new Core::IContext(this);
    icontext->setContext(context);
    icontext->setWidget(m_helpViewerForSideBar);
    Core::ICore::addContextObject(icontext);

    QAction *copy = new QAction(this);
    Core::Command *cmd = Core::ActionManager::registerAction(copy,
        Core::Constants::COPY, context);
    copy->setText(cmd->action()->text());
    copy->setIcon(cmd->action()->icon());
    connect(copy, SIGNAL(triggered()), m_helpViewerForSideBar, SLOT(copy()));

    next->setEnabled(m_helpViewerForSideBar->isForwardAvailable());
    connect(next, SIGNAL(triggered()), m_helpViewerForSideBar, SLOT(forward()));
    connect(m_helpViewerForSideBar, SIGNAL(forwardAvailable(bool)), next,
        SLOT(setEnabled(bool)));

    back->setEnabled(m_helpViewerForSideBar->isBackwardAvailable());
    connect(back, SIGNAL(triggered()), m_helpViewerForSideBar, SLOT(backward()));
    connect(m_helpViewerForSideBar, SIGNAL(backwardAvailable(bool)), back,
        SLOT(setEnabled(bool)));

    if (Core::ActionContainer *advancedMenu = Core::ActionManager::actionContainer(Core::Constants::M_EDIT_ADVANCED)) {
        // reuse TextEditor constants to avoid a second pair of menu actions
        QAction *action = new QAction(tr("Increase Font Size"), this);
        cmd = Core::ActionManager::registerAction(action, TextEditor::Constants::INCREASE_FONT_SIZE,
            context);
        connect(action, SIGNAL(triggered()), this, SLOT(scaleRightPaneUp()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);

        action = new QAction(tr("Decrease Font Size"), this);
        cmd = Core::ActionManager::registerAction(action, TextEditor::Constants::DECREASE_FONT_SIZE,
            context);
        connect(action, SIGNAL(triggered()), this, SLOT(scaleRightPaneDown()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);

        action = new QAction(tr("Reset Font Size"), this);
        cmd = Core::ActionManager::registerAction(action, TextEditor::Constants::RESET_FONT_SIZE,
            context);
        connect(action, SIGNAL(triggered()), this, SLOT(resetRightPaneScale()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);
    }

    // force setup, as we might have never switched to full help mode
    // thus the help engine might still run without collection file setup
    m_helpManager->setupGuiHelpEngine();
}

void HelpPlugin::scaleRightPaneUp()
{
    if (m_helpViewerForSideBar)
        m_helpViewerForSideBar->scaleUp();
}

void HelpPlugin::scaleRightPaneDown()
{
    if (m_helpViewerForSideBar)
        m_helpViewerForSideBar->scaleDown();
}

void HelpPlugin::resetRightPaneScale()
{
    if (m_helpViewerForSideBar)
        m_helpViewerForSideBar->resetScale();
}

void HelpPlugin::activateHelpMode()
{
    if (contextHelpOption() != Help::Constants::ExternalHelpAlways)
        Core::ModeManager::activateMode(Id(Constants::ID_MODE_HELP));
    else
        showExternalWindow();
}

void HelpPlugin::switchToHelpMode()
{
    switchToHelpMode(m_helpViewerForSideBar->source());
}

void HelpPlugin::switchToHelpMode(const QUrl &source)
{
    activateHelpMode();
    m_centralWidget->setSource(source);
    m_centralWidget->setFocus();
}

void HelpPlugin::slotHideRightPane()
{
    Core::RightPaneWidget::instance()->setShown(false);
}

void HelpPlugin::showHideSidebar()
{
    m_sideBar->setVisible(!m_sideBar->isVisible());
    onSideBarVisibilityChanged();
}

void HelpPlugin::showExternalWindow()
{
    bool firstTime = m_firstModeChange;
    doSetupIfNeeded();
    m_externalWindow->show();
    connectExternalHelpWindow();
    m_externalWindow->activateWindow();
    if (firstTime)
        Core::ICore::mainWindow()->activateWindow();
}

void HelpPlugin::modeChanged(Core::IMode *mode, Core::IMode *old)
{
    if (mode == m_mode) {
        m_oldMode = old;
        qApp->setOverrideCursor(Qt::WaitCursor);
        doSetupIfNeeded();
        qApp->restoreOverrideCursor();
    }
}

void HelpPlugin::updateSideBarSource()
{
    if (HelpViewer *viewer = m_centralWidget->currentHelpViewer()) {
        const QUrl &url = viewer->source();
        if (url.isValid())
            updateSideBarSource(url);
    }
}

void HelpPlugin::updateSideBarSource(const QUrl &newUrl)
{
    if (m_helpViewerForSideBar)
        m_helpViewerForSideBar->setSource(newUrl);
}

void HelpPlugin::updateCloseButton()
{
    Core::HelpManager *manager = Core::HelpManager::instance();
    const bool closeOnReturn = manager->customValue(QLatin1String("ReturnOnClose"),
        false).toBool();
    m_closeButton->setEnabled((OpenPagesManager::instance().pageCount() > 1)
        | closeOnReturn);
}

void HelpPlugin::fontChanged()
{
    if (!m_helpViewerForSideBar)
        createRightPaneContextViewer();

    const QHelpEngine &engine = LocalHelpManager::helpEngine();
    QFont font = qvariant_cast<QFont>(engine.customValue(QLatin1String("font"),
        m_helpViewerForSideBar->viewerFont()));

    m_helpViewerForSideBar->setFont(font);
    const int count = OpenPagesManager::instance().pageCount();
    for (int i = 0; i < count; ++i) {
        if (HelpViewer *viewer = CentralWidget::instance()->viewerAt(i))
            viewer->setViewerFont(font);
    }
}

QStackedLayout * layoutForWidget(QWidget *parent, QWidget *widget)
{
    QList<QStackedLayout*> list = parent->findChildren<QStackedLayout*>();
    foreach (QStackedLayout *layout, list) {
        const int index = layout->indexOf(widget);
        if (index >= 0)
            return layout;
    }
    return 0;
}

void HelpPlugin::contextHelpOptionChanged()
{
    doSetupIfNeeded();
    QWidget *modeWidget = m_mode->widget();
    if (modeWidget == m_splitter
        && contextHelpOption() == Help::Constants::ExternalHelpAlways) {
        if (QWidget *widget = m_splitter->parentWidget()) {
            if (QStackedLayout *layout = layoutForWidget(widget, m_splitter)) {
                const int index = layout->indexOf(m_splitter);
                layout->removeWidget(m_splitter);
                m_mode->setWidget(new QWidget);
                layout->insertWidget(index, m_mode->widget());
                m_externalWindow->setCentralWidget(m_splitter);
                m_splitter->show();

                slotHideRightPane();
                m_mode->setEnabled(false);
                m_externalHelpBar->setVisible(true);
                m_internalHelpBar->setVisible(false);
                m_externalWindow->show();
                connectExternalHelpWindow();

                if (m_oldMode && m_mode == ModeManager::currentMode())
                    ModeManager::activateMode(m_oldMode->id());
            }
        }
    } else if (modeWidget != m_splitter
        && contextHelpOption() != Help::Constants::ExternalHelpAlways) {
        QStackedLayout *wLayout = layoutForWidget(modeWidget->parentWidget(),
            modeWidget);
        if (wLayout && m_splitter->parentWidget()->layout()) {
            const int index = wLayout->indexOf(modeWidget);
            QWidget *tmp = wLayout->widget(index);
            wLayout->removeWidget(modeWidget);
            delete tmp;

            m_splitter->parentWidget()->layout()->removeWidget(m_splitter);
            m_mode->setWidget(m_splitter);
            wLayout->insertWidget(index, m_splitter);

            m_mode->setEnabled(true);
            m_externalWindow->close();
            m_sideBar->setVisible(true);
            m_internalHelpBar->setVisible(true);
            m_externalHelpBar->setVisible(false);
        }
    }
}

void HelpPlugin::setupHelpEngineIfNeeded()
{
    m_helpManager->setEngineNeedsUpdate();
    if (Core::ModeManager::currentMode() == m_mode
        || contextHelpOption() == Help::Constants::ExternalHelpAlways)
        m_helpManager->setupGuiHelpEngine();
}

HelpViewer *HelpPlugin::viewerForContextMode()
{
    if (ModeManager::currentMode()->id() == Core::Constants::MODE_WELCOME)
        ModeManager::activateMode(Core::Constants::MODE_EDIT);

    bool showSideBySide = false;
    RightPanePlaceHolder *placeHolder = RightPanePlaceHolder::current();
    switch (contextHelpOption()) {
        case Help::Constants::SideBySideIfPossible: {
            // side by side if possible
            if (IEditor *editor = EditorManager::currentEditor()) {
                if (!placeHolder || !placeHolder->isVisible()) {
                    if (!editor->widget())
                        break;
                    if (!editor->widget()->isVisible())
                        break;
                    if (editor->widget()->width() < 800)
                        break;
                }
            }
        }   // fall through
        case Help::Constants::SideBySideAlways: {
            // side by side
            showSideBySide = true;
        }   break;

        default: // help mode
            break;
    }

    if (placeHolder && showSideBySide) {
        createRightPaneContextViewer();
        RightPaneWidget::instance()->setWidget(m_rightPaneSideBarWidget);
        RightPaneWidget::instance()->setShown(true);
        return m_helpViewerForSideBar;
    }

    activateHelpMode(); // should trigger an createPage...
    HelpViewer *viewer = m_centralWidget->currentHelpViewer();
    if (!viewer)
        viewer = OpenPagesManager::instance().createPage();
    return viewer;
}

void HelpPlugin::activateContext()
{
    createRightPaneContextViewer();

    RightPanePlaceHolder *placeHolder = RightPanePlaceHolder::current();
    if (placeHolder && m_helpViewerForSideBar->hasFocus()) {
        switchToHelpMode();
        return;
    }
    if (ModeManager::currentMode() == m_mode)
        return;

    // Find out what to show
    QMap<QString, QUrl> links;
    if (IContext *context = Core::ICore::currentContextObject()) {
        m_idFromContext = context->contextHelpId();
        links = Core::HelpManager::instance()->linksForIdentifier(m_idFromContext);
        if (links.isEmpty()) {
            // Maybe this is already an URL...
            QUrl url(m_idFromContext);
            if (url.isValid())
                links.insert(m_idFromContext, m_idFromContext);
        }
    }

    if (HelpViewer* viewer = viewerForContextMode()) {
        if (links.isEmpty()) {
            // No link found or no context object
            viewer->setSource(QUrl(Help::Constants::AboutBlank));
            viewer->setHtml(tr("<html><head><title>No Documentation</title>"
                "</head><body><br/><center><b>%1</b><br/>No documentation "
                "available.</center></body></html>").arg(m_idFromContext));
        } else {
            int version = 0;
            QRegExp exp(QLatin1String("(\\d+)"));
            QUrl source = *links.begin();
            const QLatin1String qtRefDoc = QLatin1String("com.trolltech.qt");

            // workaround to show the latest Qt version
            foreach (const QUrl &tmp, links) {
                const QString &authority = tmp.authority();
                if (authority.startsWith(qtRefDoc)) {
                    if (exp.indexIn(authority) >= 0) {
                        const int tmpVersion = exp.cap(1).toInt();
                        if (tmpVersion > version) {
                            source = tmp;
                            version = tmpVersion;
                        }
                    }
                }
            }

            const QUrl &oldSource = viewer->source();
            if (source != oldSource) {
#if !defined(QT_NO_WEBKIT)
                viewer->stop();
#endif
                const QString &fragment = source.fragment();
                const bool isQtRefDoc = source.authority().startsWith(qtRefDoc);
                if (isQtRefDoc) {
                    // workaround for qt properties
                    m_idFromContext = fragment;

                    if (!m_idFromContext.isEmpty()) {
                        connect(viewer, SIGNAL(loadFinished(bool)), this,
                            SLOT(highlightSearchTerms()));
                    }
                }

                viewer->setSource(source);

                if (isQtRefDoc && !m_idFromContext.isEmpty()) {
                    if (source.toString().remove(fragment)
                        == oldSource.toString().remove(oldSource.fragment())) {
                        highlightSearchTerms();
                    }
                }
            } else {
#if !defined(QT_NO_WEBKIT)
                viewer->page()->mainFrame()->scrollToAnchor(source.fragment());
#endif
            }
            viewer->setFocus();
        }
    }
}

void HelpPlugin::activateIndex()
{
    activateHelpMode();
    m_sideBar->activateItem(m_indexItem);
}

void HelpPlugin::activateContents()
{
    activateHelpMode();
    m_sideBar->activateItem(m_contentItem);
}

void HelpPlugin::activateSearch()
{
    activateHelpMode();
    m_sideBar->activateItem(m_searchItem);
}

void HelpPlugin::activateOpenPages()
{
    activateHelpMode();
    m_sideBar->activateItem(m_openPagesItem);
}

void HelpPlugin::activateBookmarks()
{
    activateHelpMode();
    m_sideBar->activateItem(m_bookmarkItem);
}

Utils::StyledBar *HelpPlugin::createWidgetToolBar()
{
    m_filterComboBox = new QComboBox;
    m_filterComboBox->setMinimumContentsLength(15);
    connect(m_filterComboBox, SIGNAL(activated(QString)), this,
        SLOT(filterDocumentation(QString)));
    connect(m_filterComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(updateSideBarSource()));

    m_closeButton = new QToolButton();
    m_closeButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_CLOSE_DOCUMENT)));
    m_closeButton->setToolTip(tr("Close current page"));
    connect(m_closeButton, SIGNAL(clicked()), &OpenPagesManager::instance(),
        SLOT(closeCurrentPage()));
    connect(&OpenPagesManager::instance(), SIGNAL(pagesChanged()), this,
        SLOT(updateCloseButton()));

    Utils::StyledBar *toolBar = new Utils::StyledBar;

    QHBoxLayout *layout = new QHBoxLayout(toolBar);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(OpenPagesManager::instance().openPagesComboBox(), 10);
    layout->addSpacing(5);
    layout->addWidget(new QLabel(tr("Filtered by:")));
    layout->addWidget(m_filterComboBox);
    layout->addStretch();
    layout->addWidget(m_closeButton);

    return toolBar;
}

Utils::StyledBar *HelpPlugin::createIconToolBar(bool external)
{
    Utils::StyledBar *toolBar = new Utils::StyledBar;
    toolBar->setVisible(false);

    QAction *home, *back, *next, *bookmark;
    if (external) {
        home = new QAction(QIcon(QLatin1String(IMAGEPATH "home.png")),
            tr("Home"), toolBar);
        connect(home, SIGNAL(triggered()), m_centralWidget, SLOT(home()));

        back = new QAction(QIcon(QLatin1String(IMAGEPATH "previous.png")),
            tr("Previous Page"), toolBar);
        back->setEnabled(m_centralWidget->isBackwardAvailable());
        connect(back, SIGNAL(triggered()), m_centralWidget, SLOT(backward()));
        connect(m_centralWidget, SIGNAL(backwardAvailable(bool)), back,
            SLOT(setEnabled(bool)));

        next = new QAction(QIcon(QLatin1String(IMAGEPATH "next.png")),
            tr("Next Page"), toolBar);
        next->setEnabled(m_centralWidget->isForwardAvailable());
        connect(next, SIGNAL(triggered()), m_centralWidget, SLOT(forward()));
        connect(m_centralWidget, SIGNAL(forwardAvailable(bool)), next,
            SLOT(setEnabled(bool)));

        bookmark = new QAction(QIcon(QLatin1String(IMAGEPATH "bookmark.png")),
            tr("Add Bookmark"), toolBar);
        connect(bookmark, SIGNAL(triggered()), this, SLOT(addBookmark()));
    } else {
        home = Core::ActionManager::command(Core::Id("Help.Home"))->action();
        back = Core::ActionManager::command(Core::Id("Help.Previous"))->action();
        next = Core::ActionManager::command(Core::Id("Help.Next"))->action();
        bookmark = Core::ActionManager::command(Core::Id("Help.AddBookmark"))->action();
    }

    setupNavigationMenus(back, next, toolBar);

    QHBoxLayout *layout = new QHBoxLayout(toolBar);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(toolButton(home));
    layout->addWidget(toolButton(back));
    layout->addWidget(toolButton(next));
    layout->addWidget(new Utils::StyledSeparator(toolBar));
    layout->addWidget(toolButton(bookmark));
    layout->addWidget(new Utils::StyledSeparator(toolBar));

    return toolBar;
}

void HelpPlugin::updateFilterComboBox()
{
    const QHelpEngine &engine = LocalHelpManager::helpEngine();
    QString curFilter = m_filterComboBox->currentText();
    if (curFilter.isEmpty())
        curFilter = engine.currentFilter();
    m_filterComboBox->clear();
    m_filterComboBox->addItems(engine.customFilters());
    int idx = m_filterComboBox->findText(curFilter);
    if (idx < 0)
        idx = 0;
    m_filterComboBox->setCurrentIndex(idx);
}

void HelpPlugin::filterDocumentation(const QString &customFilter)
{
    LocalHelpManager::helpEngine().setCurrentFilter(customFilter);
}

void HelpPlugin::addBookmark()
{
    HelpViewer *viewer = m_centralWidget->currentHelpViewer();

    const QString &url = viewer->source().toString();
    if (url.isEmpty() || url == Help::Constants::AboutBlank)
        return;

    BookmarkManager *manager = &LocalHelpManager::bookmarkManager();
    manager->showBookmarkDialog(m_centralWidget, viewer->title(), url);
}

void HelpPlugin::highlightSearchTerms()
{
    if (HelpViewer* viewer = viewerForContextMode()) {
        disconnect(viewer, SIGNAL(loadFinished(bool)), this,
            SLOT(highlightSearchTerms()));

#if !defined(QT_NO_WEBKIT)
        const QWebElement &document = viewer->page()->mainFrame()->documentElement();
        const QWebElementCollection &collection = document.findAll(QLatin1String("h3.fn a"));

        const QLatin1String property("background-color");
        foreach (const QWebElement &element, collection) {
            const QString &name = element.attribute(QLatin1String("name"));
            if (name.isEmpty())
                continue;

            if (m_oldAttrValue == name
                || name.startsWith(m_oldAttrValue + QLatin1Char('-'))) {
                QWebElement parent = element.parent();
                parent.setStyleProperty(property, m_styleProperty);
            }

            if (m_idFromContext == name
                || name.startsWith(m_idFromContext + QLatin1Char('-'))) {
                QWebElement parent = element.parent();
                m_styleProperty = parent.styleProperty(property,
                    QWebElement::ComputedStyle);
                parent.setStyleProperty(property, QLatin1String("yellow"));
            }
        }
        m_oldAttrValue = m_idFromContext;
#endif
    }
}

void HelpPlugin::handleHelpRequest(const QUrl &url)
{
    if (HelpViewer::launchWithExternalApp(url))
        return;

    QString address = url.toString();
    if (!Core::HelpManager::instance()->findFile(url).isValid()) {
        if (address.startsWith(QLatin1String("qthelp://com.nokia."))
            || address.startsWith(QLatin1String("qthelp://com.trolltech."))) {
                // local help not installed, resort to external web help
                QString urlPrefix = QLatin1String("http://doc.qt.digia.com/");
                if (url.authority() == QLatin1String("com.nokia.qtcreator"))
                    urlPrefix.append(QString::fromLatin1("qtcreator"));
                else
                    urlPrefix.append(QLatin1String("latest"));
            address = urlPrefix + address.mid(address.lastIndexOf(QLatin1Char('/')));
        }
    }

    const QUrl newUrl(address);
    if (newUrl.queryItemValue(QLatin1String("view")) == QLatin1String("split")) {
        if (HelpViewer* viewer = viewerForContextMode())
            viewer->setSource(newUrl);
    } else {
        switchToHelpMode(newUrl);
    }
}

void HelpPlugin::slotAboutToShowBackMenu()
{
#if !defined(QT_NO_WEBKIT)
    m_backMenu->clear();
    if (QWebHistory *history = viewerForContextMode()->history()) {
        const int currentItemIndex = history->currentItemIndex();
        QList<QWebHistoryItem> items = history->backItems(history->count());
        for (int i = items.count() - 1; i >= 0; --i) {
            QAction *action = new QAction(this);
            action->setText(items.at(i).title());
            action->setData(-1 * (currentItemIndex - i));
            m_backMenu->addAction(action);
        }
    }
#endif
}

void HelpPlugin::slotAboutToShowNextMenu()
{
#if !defined(QT_NO_WEBKIT)
    m_nextMenu->clear();
    if (QWebHistory *history = viewerForContextMode()->history()) {
        const int count = history->count();
        QList<QWebHistoryItem> items = history->forwardItems(count);
        for (int i = 0; i < items.count(); ++i) {
            QAction *action = new QAction(this);
            action->setData(count - i);
            action->setText(items.at(i).title());
            m_nextMenu->addAction(action);
        }
    }
#endif
}

void HelpPlugin::slotOpenActionUrl(QAction *action)
{
#if !defined(QT_NO_WEBKIT)
    if (HelpViewer* viewer = viewerForContextMode()) {
        const int offset = action->data().toInt();
        QWebHistory *history = viewer->history();
        if (offset > 0) {
            history->goToItem(history->forwardItems(history->count()
                - offset + 1).back());  // forward
        } else if (offset < 0) {
            history->goToItem(history->backItems(-1 * offset).first()); // back
        }
    }
#else
    Q_UNUSED(action)
#endif
}

void HelpPlugin::slotOpenSupportPage()
{
    switchToHelpMode(QUrl(QLatin1String("qthelp://com.nokia.qtcreator/doc/technical-support.html")));
}

void HelpPlugin::slotReportBug()
{
    QDesktopServices::openUrl(QUrl(QLatin1String("https://bugreports.qt-project.org")));
}

void HelpPlugin::openFindToolBar()
{
    if (Find::FindPlugin::instance())
        Find::FindPlugin::instance()->openFindToolBar(Find::FindPlugin::FindForward);
}

void  HelpPlugin::onSideBarVisibilityChanged()
{
    m_isSidebarVisible = m_sideBar->isVisible();
    m_toggleSideBarAction->setToolTip(m_isSidebarVisible ? tr("Hide Sidebar") : tr("Show Sidebar"));
}

void HelpPlugin::doSetupIfNeeded()
{
    m_helpManager->setupGuiHelpEngine();
    if (m_firstModeChange) {
        qApp->processEvents();
        setupUi();
        resetFilter();
        m_firstModeChange = false;
        OpenPagesManager::instance().setupInitialPages();
    }
}

int HelpPlugin::contextHelpOption() const
{
    QSettings *settings = Core::ICore::settings();
    const QString key = QLatin1String(Help::Constants::ID_MODE_HELP) + QLatin1String("/ContextHelpOption");
    if (settings->contains(key))
        return settings->value(key, Help::Constants::SideBySideIfPossible).toInt();

    const QHelpEngineCore &engine = LocalHelpManager::helpEngine();
    return engine.customValue(QLatin1String("ContextHelpOption"),
        Help::Constants::SideBySideIfPossible).toInt();
}

void HelpPlugin::connectExternalHelpWindow()
{
    if (m_connectWindow) {
        m_connectWindow = false;
        connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()),
            m_externalWindow, SLOT(close()));
        connect(m_externalWindow, SIGNAL(activateIndex()), this,
            SLOT(activateIndex()));
        connect(m_externalWindow, SIGNAL(activateContents()), this,
            SLOT(activateContents()));
        connect(m_externalWindow, SIGNAL(activateSearch()), this,
            SLOT(activateSearch()));
        connect(m_externalWindow, SIGNAL(activateBookmarks()), this,
            SLOT(activateBookmarks()));
        connect(m_externalWindow, SIGNAL(activateOpenPages()), this,
            SLOT(activateOpenPages()));
        connect(m_externalWindow, SIGNAL(addBookmark()), this,
            SLOT(addBookmark()));
        connect(m_externalWindow, SIGNAL(showHideSidebar()), this,
            SLOT(showHideSidebar()));
    }
}

void HelpPlugin::setupNavigationMenus(QAction *back, QAction *next, QWidget *parent)
{
#if !defined(QT_NO_WEBKIT)
    if (!m_backMenu) {
        m_backMenu = new QMenu(parent);
        connect(m_backMenu, SIGNAL(aboutToShow()), this,
            SLOT(slotAboutToShowBackMenu()));
        connect(m_backMenu, SIGNAL(triggered(QAction*)), this,
            SLOT(slotOpenActionUrl(QAction*)));
    }

    if (!m_nextMenu) {
        m_nextMenu = new QMenu(parent);
        connect(m_nextMenu, SIGNAL(aboutToShow()), this,
            SLOT(slotAboutToShowNextMenu()));
        connect(m_nextMenu, SIGNAL(triggered(QAction*)), this,
            SLOT(slotOpenActionUrl(QAction*)));
    }

    back->setMenu(m_backMenu);
    next->setMenu(m_nextMenu);
#else
    Q_UNUSED(back)
    Q_UNUSED(next)
    Q_UNUSED(parent)
#endif
}

Q_EXPORT_PLUGIN(HelpPlugin)
