/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "helpplugin.h"

#include "bookmarkmanager.h"
#include "centralwidget.h"
#include "contentwindow.h"
#include "docsettingspage.h"
#include "filtersettingspage.h"
#include "generalsettingspage.h"
#include "helpconstants.h"
#include "helpfindsupport.h"
#include "helpindexfilter.h"
#include "helpmanager.h"
#include "helpmode.h"
#include "helpviewer.h"
#include "indexwindow.h"
#include "openpagesmanager.h"
#include "openpagesmodel.h"
#include "searchwidget.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
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
#include <coreplugin/uniqueidmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditorconstants.h>
#include <utils/styledbar.h>
#include <welcome/welcomemode.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLibraryInfo>
#include <QtCore/QTranslator>
#include <QtCore/qplugin.h>

#include <QtGui/QAction>
#include <QtGui/QComboBox>
#include <QtGui/QDesktopServices>
#include <QtGui/QShortcut>
#include <QtGui/QSplitter>
#include <QtGui/QToolBar>

#include <QtHelp/QHelpEngine>

#if !defined(QT_NO_WEBKIT)
#include <QtWebKit/QWebElement>
#include <QtWebKit/QWebElementCollection>
#include <QtWebKit/QWebFrame>
#endif

using namespace Core::Constants;
using namespace Help::Internal;

const char * const SB_INDEX = "Index";
const char * const SB_CONTENTS = "Contents";
const char * const SB_BOOKMARKS = "Bookmarks";
const char * const SB_SEARCH = "Search";
const char * const SB_OPENPAGES = "OpenPages";

#define IMAGEPATH ":/help/images/"
#if defined(Q_OS_MAC)
#   define DOCPATH "/../Resources/doc/"
#else
#   define DOCPATH "/../share/doc/qtcreator/"
#endif

HelpPlugin::HelpPlugin()
    : m_mode(0),
    m_core(0),
    m_centralWidget(0),
    m_helpViewerForSideBar(0),
    m_contentItem(0),
    m_indexItem(0),
    m_searchItem(0),
    m_bookmarkItem(0),
    m_sideBar(0),
    m_firstModeChange(true)
{
}

HelpPlugin::~HelpPlugin()
{
}

bool HelpPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error)
    m_core = Core::ICore::instance();
    QList<int> globalcontext;
    globalcontext << Core::Constants::C_GLOBAL_ID;
    QList<int> modecontext;
    modecontext << m_core->uniqueIDManager()->uniqueIdentifier(Constants::C_MODE_HELP);

    const QString &locale = qApp->property("qtc_locale").toString();
    if (!locale.isEmpty()) {
        QTranslator *qtr = new QTranslator(this);
        QTranslator *qhelptr = new QTranslator(this);
        const QString &creatorTrPath = Core::ICore::instance()->resourcePath()
            + QLatin1String("/translations");
        const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString &trFile = QLatin1String("assistant_") + locale;
        const QString &helpTrFile = QLatin1String("qt_help_") + locale;
        if (qtr->load(trFile, qtTrPath) || qtr->load(trFile, creatorTrPath))
            qApp->installTranslator(qtr);
        if (qhelptr->load(helpTrFile, qtTrPath) || qhelptr->load(helpTrFile, creatorTrPath))
            qApp->installTranslator(qhelptr);
    }

    addAutoReleasedObject(m_helpManager = new LocalHelpManager(this));
    addAutoReleasedObject(m_openPagesManager = new OpenPagesManager(this));
    addAutoReleasedObject(m_docSettingsPage = new DocSettingsPage());
    addAutoReleasedObject(m_filterSettingsPage = new FilterSettingsPage());
    addAutoReleasedObject(m_generalSettingsPage = new GeneralSettingsPage());

    connect(m_generalSettingsPage, SIGNAL(fontChanged()), this,
        SLOT(fontChanged()));
    connect(Core::HelpManager::instance(), SIGNAL(helpRequested(QUrl)), this,
        SLOT(handleHelpRequest(QUrl)));
    m_filterSettingsPage->setHelpManager(m_helpManager);
    connect(m_filterSettingsPage, SIGNAL(filtersChanged()), this,
        SLOT(setupHelpEngineIfNeeded()));
    connect(Core::HelpManager::instance(), SIGNAL(documentationChanged()), this,
        SLOT(setupHelpEngineIfNeeded()));

    m_splitter = new Core::MiniSplitter;
    m_centralWidget = new Help::Internal::CentralWidget();
    connect(m_centralWidget, SIGNAL(sourceChanged(QUrl)), this,
        SLOT(updateSideBarSource(QUrl)));

    // Add Home, Previous and Next actions (used in the toolbar)
    QAction *action = new QAction(QIcon(QLatin1String(IMAGEPATH "home.png")),
        tr("Home"), this);
    Core::ActionManager *am = m_core->actionManager();
    Core::Command *cmd = am->registerAction(action, QLatin1String("Help.Home"),
        globalcontext);
    connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(home()));

    action = new QAction(QIcon(QLatin1String(IMAGEPATH "previous.png")),
        tr("Previous Page"), this);
    cmd = am->registerAction(action, QLatin1String("Help.Previous"), modecontext);
    cmd->setDefaultKeySequence(QKeySequence::Back);
    action->setEnabled(m_centralWidget->isBackwardAvailable());
    connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(backward()));
    connect(m_centralWidget, SIGNAL(backwardAvailable(bool)), action,
        SLOT(setEnabled(bool)));

    action = new QAction(QIcon(QLatin1String(IMAGEPATH "next.png")), tr("Next Page"),
        this);
    cmd = am->registerAction(action, QLatin1String("Help.Next"), modecontext);
    cmd->setDefaultKeySequence(QKeySequence::Forward);
    action->setEnabled(m_centralWidget->isForwardAvailable());
    connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(forward()));
    connect(m_centralWidget, SIGNAL(forwardAvailable(bool)), action,
        SLOT(setEnabled(bool)));

    action = new QAction(QIcon(QLatin1String(IMAGEPATH "bookmark.png")),
        tr("Add Bookmark"), this);
    cmd = am->registerAction(action, QLatin1String("Help.AddBookmark"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::Key_M));
    connect(action, SIGNAL(triggered()), this, SLOT(addBookmark()));

    // Add Contents, Index, and Context menu items and a separator to the Help menu
    action = new QAction(QIcon::fromTheme(QLatin1String("help-contents")), tr("Contents"), this);
    cmd = am->registerAction(action, QLatin1String("Help.Contents"), globalcontext);
    am->actionContainer(M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, SIGNAL(triggered()), this, SLOT(activateContents()));

    action = new QAction(tr("Index"), this);
    cmd = am->registerAction(action, QLatin1String("Help.Index"), globalcontext);
    am->actionContainer(M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, SIGNAL(triggered()), this, SLOT(activateIndex()));

    action = new QAction(tr("Context Help"), this);
    cmd = am->registerAction(action, QLatin1String("Help.Context"), globalcontext);
    am->actionContainer(M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F1));
    connect(action, SIGNAL(triggered()), this, SLOT(activateContext()));

#ifndef Q_WS_MAC
    action = new QAction(this);
    action->setSeparator(true);
    cmd = am->registerAction(action, QLatin1String("Help.Separator"), globalcontext);
    am->actionContainer(M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
#endif

    action = new QAction(this);
    am->registerAction(action, Core::Constants::PRINT, modecontext);
    connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(print()));

    action = new QAction(this);
    cmd = am->registerAction(action, Core::Constants::COPY, modecontext);
    connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(copy()));
    action->setText(cmd->action()->text());
    action->setIcon(cmd->action()->icon());

    if (Core::ActionContainer *advancedMenu = am->actionContainer(M_EDIT_ADVANCED)) {
        // reuse TextEditor constants to avoid a second pair of menu actions
        action = new QAction(tr("Increase Font Size"), this);
        cmd = am->registerAction(action, TextEditor::Constants::INCREASE_FONT_SIZE,
            modecontext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl++")));
        connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(zoomIn()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);

        action = new QAction(tr("Decrease Font Size"), this);
        cmd = am->registerAction(action, TextEditor::Constants::DECREASE_FONT_SIZE,
            modecontext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+-")));
        connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(zoomOut()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);

        action = new QAction(tr("Reset Font Size"), this);
        cmd = am->registerAction(action, TextEditor::Constants::RESET_FONT_SIZE,
            modecontext);
#ifndef Q_WS_MAC
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+0")));
#endif
        connect(action, SIGNAL(triggered()), m_centralWidget, SLOT(resetZoom()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);
    }

    if (Core::ActionContainer *windowMenu = am->actionContainer(M_WINDOW)) {
        // reuse EditorManager constants to avoid a second pair of menu actions
        action = new QAction(QApplication::tr("EditorManager",
            "Next Open Document in History"), this);
        Core::Command *ctrlTab = am->registerAction(action, GOTOPREVINHISTORY,
            modecontext);   // Goto Previous In History Action
        windowMenu->addAction(ctrlTab, Core::Constants::G_WINDOW_NAVIGATE);
        connect(action, SIGNAL(triggered()), &OpenPagesManager::instance(),
            SLOT(gotoPreviousPage()));

        action = new QAction(QApplication::tr("EditorManager",
            "Previous Open Document in History"), this);
        Core::Command *ctrlShiftTab = am->registerAction(action, GOTONEXTINHISTORY,
            modecontext);   // Goto Next In History Action
        windowMenu->addAction(ctrlShiftTab, Core::Constants::G_WINDOW_NAVIGATE);
        connect(action, SIGNAL(triggered()), &OpenPagesManager::instance(),
            SLOT(gotoNextPage()));

#ifdef Q_WS_MAC
        ctrlTab->setDefaultKeySequence(QKeySequence(tr("Alt+Tab")));
        ctrlShiftTab->setDefaultKeySequence(QKeySequence(tr("Alt+Shift+Tab")));
#else
        ctrlTab->setDefaultKeySequence(QKeySequence(tr("Ctrl+Tab")));
        ctrlShiftTab->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+Tab")));
#endif
    }

    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(m_centralWidget);
    agg->add(new HelpFindSupport(m_centralWidget));
    m_mainWidget = new QWidget;
    m_splitter->addWidget(m_mainWidget);
    QVBoxLayout *mainWidgetLayout = new QVBoxLayout(m_mainWidget);
    mainWidgetLayout->setMargin(0);
    mainWidgetLayout->setSpacing(0);
    mainWidgetLayout->addWidget(createToolBar());
    mainWidgetLayout->addWidget(m_centralWidget);

    HelpIndexFilter *helpIndexFilter = new HelpIndexFilter();
    addAutoReleasedObject(helpIndexFilter);
    connect(helpIndexFilter, SIGNAL(linkActivated(QUrl)), this,
        SLOT(switchToHelpMode(QUrl)));
    connect(helpIndexFilter, SIGNAL(linksActivated(QMap<QString, QUrl>, QString)),
        this, SLOT(switchToHelpMode(QMap<QString, QUrl>, QString)));

    QDesktopServices::setUrlHandler("qthelp", this, "handleHelpRequest");
    connect(m_core->modeManager(), SIGNAL(currentModeChanged(Core::IMode*)),
        this, SLOT(modeChanged(Core::IMode*)));

    addAutoReleasedObject(m_mode = new HelpMode(m_splitter, m_centralWidget));
    m_mode->setContext(QList<int>() << modecontext);

    return true;
}

void HelpPlugin::extensionsInitialized()
{
    const QString &nsInternal = QString::fromLatin1("com.nokia.qtcreator.%1%2%3")
        .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR).arg(IDE_VERSION_RELEASE);

    Core::HelpManager *helpManager = Core::HelpManager::instance();
    foreach (const QString &ns, helpManager->registeredNamespaces()) {
        if (ns.startsWith(QLatin1String("com.nokia.qtcreator."))
            && ns != nsInternal)
            helpManager->unregisterDocumentation(QStringList() << ns);
    }

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
    helpManager->registerDocumentation(filesToRegister);
}

void HelpPlugin::aboutToShutdown()
{
    if (m_sideBar)
        m_sideBar->saveSettings(m_core->settings(), QLatin1String("HelpSideBar"));
}

void HelpPlugin::setupUi()
{
    // side bar widgets and shortcuts
    QList<int> modecontext;
    Core::ActionManager *am = m_core->actionManager();
    modecontext << m_core->uniqueIDManager()->uniqueIdentifier(Constants::C_MODE_HELP);

    IndexWindow *indexWindow = new IndexWindow();
    indexWindow->setWindowTitle(tr("Index"));
    m_indexItem = new Core::SideBarItem(indexWindow, QLatin1String(SB_INDEX));

    connect(indexWindow, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));
    connect(indexWindow, SIGNAL(linksActivated(QMap<QString, QUrl>, QString)),
        m_centralWidget, SLOT(showTopicChooser(QMap<QString, QUrl>, QString)));

    QMap<QString, Core::Command*> shortcutMap;
    QShortcut *shortcut = new QShortcut(m_splitter);
    shortcut->setWhatsThis(tr("Activate Index in Help mode"));
    Core::Command* cmd = am->registerShortcut(shortcut,
        QLatin1String("Help.IndexShortcut"), modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateIndex()));
    shortcutMap.insert(QLatin1String(SB_INDEX), cmd);

    ContentWindow *contentWindow = new ContentWindow();
    contentWindow->setWindowTitle(tr("Contents"));
    m_contentItem = new Core::SideBarItem(contentWindow, QLatin1String(SB_CONTENTS));
    connect(contentWindow, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));

    shortcut = new QShortcut(m_splitter);
    shortcut->setWhatsThis(tr("Activate Contents in Help mode"));
    cmd = am->registerShortcut(shortcut, QLatin1String("Help.ContentsShortcut"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateContents()));
    shortcutMap.insert(QLatin1String(SB_CONTENTS), cmd);

    SearchWidget *searchWidget = new SearchWidget();
    searchWidget->setWindowTitle(tr("Search"));
    m_searchItem = new Core::SideBarItem(searchWidget, "Search");
    connect(searchWidget, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSourceFromSearch(QUrl)));

     shortcut = new QShortcut(m_splitter);
     shortcut->setWhatsThis(tr("Activate Search in Help mode"));
     cmd = am->registerShortcut(shortcut, QLatin1String("Help.SearchShortcut"),
         modecontext);
     cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Slash));
     connect(shortcut, SIGNAL(activated()), this, SLOT(activateSearch()));
     shortcutMap.insert(SB_SEARCH, cmd);

    BookmarkManager *manager = &LocalHelpManager::bookmarkManager();
    BookmarkWidget *bookmarkWidget = new BookmarkWidget(manager, 0, false);
    bookmarkWidget->setWindowTitle(tr("Bookmarks"));
    m_bookmarkItem = new Core::SideBarItem(bookmarkWidget, QLatin1String(SB_BOOKMARKS));
    connect(bookmarkWidget, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));

     shortcut = new QShortcut(m_splitter);
     shortcut->setWhatsThis(tr("Activate Bookmarks in Help mode"));
     cmd = am->registerShortcut(shortcut, QLatin1String("Help.BookmarkShortcut"),
         modecontext);
     cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
     connect(shortcut, SIGNAL(activated()), this, SLOT(activateBookmarks()));
     shortcutMap.insert(SB_BOOKMARKS, cmd);

    QWidget *openPagesWidget = OpenPagesManager::instance().openPagesWidget();
    openPagesWidget->setWindowTitle(tr("Open Pages"));
    m_openPagesItem = new Core::SideBarItem(openPagesWidget, QLatin1String(SB_OPENPAGES));

    shortcut = new QShortcut(m_splitter);
    shortcut->setWhatsThis(tr("Activate Open Pages in Help mode"));
    cmd = am->registerShortcut(shortcut, QLatin1String("Help.PagesShortcut"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateOpenPages()));
    shortcutMap.insert(QLatin1String(SB_OPENPAGES), cmd);

    QList<Core::SideBarItem*> itemList;
    itemList << m_contentItem << m_indexItem << m_searchItem << m_bookmarkItem
        << m_openPagesItem;
    m_sideBar = new Core::SideBar(itemList, QList<Core::SideBarItem*>()
        << m_contentItem << m_openPagesItem);
    m_sideBar->setShortcutMap(shortcutMap);

    m_splitter->setOpaqueResize(false);
    m_splitter->insertWidget(0, m_sideBar);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes(QList<int>() << 300 << 300);
    m_sideBar->readSettings(m_core->settings(), QLatin1String("HelpSideBar"));
}

void HelpPlugin::resetFilter()
{
    const QString &filterInternal = QString::fromLatin1("Qt Creator %1.%2.%3")
        .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR).arg(IDE_VERSION_RELEASE);
    const QRegExp filterRegExp(QLatin1String("Qt Creator \\d*\\.\\d*\\.\\d*"));

    QHelpEngineCore *engine = &LocalHelpManager::helpEngine();
    const QStringList &filters = engine->customFilters();
    foreach (const QString &filter, filters) {
        if (filterRegExp.exactMatch(filter) && filter != filterInternal)
            engine->removeCustomFilter(filter);
    }

    const QLatin1String weAddedFilterKey("UnfilteredFilterInserted");
    const QLatin1String previousFilterNameKey("UnfilteredFilterName");
    if (engine->customValue(weAddedFilterKey).toInt() == 1) {
        // we added a filter at some point, remove previously added filter
        const QString &filter = engine->customValue(previousFilterNameKey).toString();
        if (!filter.isEmpty())
            engine->removeCustomFilter(filter);
    }

    // potentially remove a filter with new name
    const QString filterName = tr("Unfiltered");
    engine->removeCustomFilter(filterName);
    engine->addCustomFilter(filterName, QStringList());
    engine->setCustomValue(weAddedFilterKey, 1);
    engine->setCustomValue(previousFilterNameKey, filterName);
    engine->setCurrentFilter(filterName);

    updateFilterComboBox();
    connect(engine, SIGNAL(setupFinished()), this, SLOT(updateFilterComboBox()));
}

void HelpPlugin::createRightPaneContextViewer()
{
    if (m_helpViewerForSideBar)
        return;

    QAction *switchToHelp = new QAction(tr("Go to Help Mode"), this);
    connect(switchToHelp, SIGNAL(triggered()), this, SLOT(switchToHelpMode()));

    QAction *next = new QAction(QIcon(QLatin1String(IMAGEPATH "next.png")),
        tr("Next"), this);
    QAction *previous = new QAction(QIcon(QLatin1String(IMAGEPATH "previous.png")),
        tr("Previous"), this);

    // Dummy layout to align the close button to the right
    QHBoxLayout *hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(0);
    hboxLayout->setMargin(0);

    // left side actions
    QToolBar *rightPaneToolBar = new QToolBar();
    rightPaneToolBar->addAction(switchToHelp);
    rightPaneToolBar->addAction(previous);
    rightPaneToolBar->addAction(next);

    hboxLayout->addWidget(rightPaneToolBar);
    hboxLayout->addStretch();

    QToolButton *closeButton = new QToolButton();
    closeButton->setIcon(QIcon(":/core/images/closebutton.png"));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(slotHideRightPane()));

    // close button to the right
    hboxLayout->addWidget(closeButton);

    QVBoxLayout *rightPaneLayout = new QVBoxLayout;
    rightPaneLayout->setMargin(0);
    rightPaneLayout->setSpacing(0);

    QWidget *rightPaneSideBar = new QWidget;
    rightPaneSideBar->setLayout(rightPaneLayout);
    addAutoReleasedObject(new Core::BaseRightPaneWidget(rightPaneSideBar));

    Utils::StyledBar *rightPaneStyledBar = new Utils::StyledBar;
    rightPaneStyledBar->setLayout(hboxLayout);
    rightPaneLayout->addWidget(rightPaneStyledBar);

    m_helpViewerForSideBar = new HelpViewer(qreal(0.0), rightPaneSideBar);
    rightPaneLayout->addWidget(m_helpViewerForSideBar);
    rightPaneLayout->addWidget(new Core::FindToolBarPlaceHolder(rightPaneSideBar));
    rightPaneSideBar->setFocusProxy(m_helpViewerForSideBar);

    Aggregation::Aggregate *agg = new Aggregation::Aggregate();
    agg->add(m_helpViewerForSideBar);
    agg->add(new HelpViewerFindSupport(m_helpViewerForSideBar));
    m_core->addContextObject(new Core::BaseContext(m_helpViewerForSideBar, QList<int>()
        << m_core->uniqueIDManager()->uniqueIdentifier(Constants::C_HELP_SIDEBAR), this));

    QAction *copy = new QAction(this);
    Core::Command *cmd = m_core->actionManager()->registerAction(copy, Core::Constants::COPY,
        QList<int>() << m_core->uniqueIDManager()->uniqueIdentifier(Constants::C_HELP_SIDEBAR));
    copy->setText(cmd->action()->text());
    copy->setIcon(cmd->action()->icon());

    connect(copy, SIGNAL(triggered()), m_helpViewerForSideBar, SLOT(copy()));
    connect(next, SIGNAL(triggered()), m_helpViewerForSideBar, SLOT(forward()));
    connect(previous, SIGNAL(triggered()), m_helpViewerForSideBar, SLOT(backward()));

    // force setup, as we might have never switched to full help mode
    // thus the help engine might still run without collection file setup
    m_helpManager->setupGuiHelpEngine();
}

void HelpPlugin::activateHelpMode()
{
    m_core->modeManager()->activateMode(QLatin1String(Constants::ID_MODE_HELP));
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

void HelpPlugin::switchToHelpMode(const QMap<QString, QUrl> &urls,
    const QString &keyword)
{
    activateHelpMode();
    m_centralWidget->showTopicChooser(urls, keyword);
}

void HelpPlugin::slotHideRightPane()
{
    Core::RightPaneWidget::instance()->setShown(false);
}

void HelpPlugin::modeChanged(Core::IMode *mode)
{
    if (mode == m_mode && m_firstModeChange) {
        m_firstModeChange = false;

        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        m_helpManager->setupGuiHelpEngine();
        setupUi();
        resetFilter();
        OpenPagesManager::instance().setupInitialPages();

        qApp->restoreOverrideCursor();
    } else if (mode == m_mode && !m_firstModeChange) {
        qApp->setOverrideCursor(Qt::WaitCursor);
        m_helpManager->setupGuiHelpEngine();
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
    m_closeButton->setEnabled(OpenPagesManager::instance().pageCount() > 1);
}

void HelpPlugin::fontChanged()
{
    if (!m_helpViewerForSideBar)
        createRightPaneContextViewer();

    const QHelpEngine &engine = LocalHelpManager::helpEngine();
    QFont font = qVariantValue<QFont>(engine.customValue(QLatin1String("font"),
        m_helpViewerForSideBar->viewerFont()));

    m_helpViewerForSideBar->setFont(font);
    const int count = OpenPagesManager::instance().pageCount();
    for (int i = 0; i < count; ++i) {
        if (HelpViewer *viewer = CentralWidget::instance()->viewerAt(i))
            viewer->setViewerFont(font);
    }
}

void HelpPlugin::setupHelpEngineIfNeeded()
{
    m_helpManager->setEngineNeedsUpdate();
    if (Core::ModeManager::instance()->currentMode() == m_mode)
        m_helpManager->setupGuiHelpEngine();
}

HelpViewer* HelpPlugin::viewerForContextMode()
{
    using namespace Core;

    bool showSideBySide = false;
    const QHelpEngineCore &engine = LocalHelpManager::helpEngine();
    RightPanePlaceHolder *placeHolder = RightPanePlaceHolder::current();
    switch (engine.customValue(QLatin1String("ContextHelpOption"), 0).toInt()) {
        case 0: {
            // side by side if possible
            if (IEditor *editor = EditorManager::instance()->currentEditor()) {
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
        case 1: {
            // side by side
            showSideBySide = true;
        }   break;

        default: // help mode
            break;
    }

    HelpViewer *viewer = m_centralWidget->currentHelpViewer();
    if (placeHolder && showSideBySide) {
        RightPaneWidget::instance()->setShown(true);

        createRightPaneContextViewer();
        viewer = m_helpViewerForSideBar;
    } else {
        activateHelpMode();
        if (!viewer)
            viewer = OpenPagesManager::instance().createPage();
    }
    return viewer;
}

void HelpPlugin::activateContext()
{
    using namespace Core;
    createRightPaneContextViewer();

    RightPanePlaceHolder* placeHolder = RightPanePlaceHolder::current();
    if (placeHolder && m_helpViewerForSideBar->hasFocus()) {
        switchToHelpMode();
        return;
    } else if (m_core->modeManager()->currentMode() == m_mode)
        return;

    // Find out what to show
    QMap<QString, QUrl> links;
    if (IContext *context = m_core->currentContextObject()) {
        m_idFromContext = context->contextHelpId();
        links = Core::HelpManager::instance()->linksForIdentifier(m_idFromContext);
    }

    if (HelpViewer* viewer = viewerForContextMode()) {
        if (links.isEmpty()) {
            // No link found or no context object
            viewer->setHtml(tr("<html><head><title>No Documentation</title>"
                "</head><body><br/><center><b>%1</b><br/>No documentation "
                "available.</center></body></html>").arg(m_idFromContext));
            viewer->setSource(QUrl());
        } else {
            const QUrl &source = *links.begin();
            const QUrl &oldSource = viewer->source();
            if (source != oldSource) {
#if !defined(QT_NO_WEBKIT)
                viewer->stop();
#endif
                viewer->setSource(source);
                connect(viewer, SIGNAL(loadFinished(bool)), this,
                    SLOT(highlightSearchTerms()));

                if (source.toString().remove(source.fragment())
                    == oldSource.toString().remove(oldSource.fragment())) {
                        highlightSearchTerms();
                }
            } else {
#if !defined(QT_NO_WEBKIT)
                viewer->page()->mainFrame()->scrollToAnchor(source.fragment());
#endif
            }
        }
        viewer->setFocus();
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

QToolBar *HelpPlugin::createToolBar()
{
    QToolBar *toolWidget = new QToolBar;
    Core::ActionManager *am = m_core->actionManager();
    toolWidget->addAction(am->command(QLatin1String("Help.Home"))->action());
    toolWidget->addAction(am->command(QLatin1String("Help.Previous"))->action());
    toolWidget->addAction(am->command(QLatin1String("Help.Next"))->action());
    toolWidget->addSeparator();
    toolWidget->addAction(am->command(QLatin1String("Help.AddBookmark"))->action());
    toolWidget->setMovable(false);

    toolWidget->addSeparator();

    QWidget *w = new QWidget;
    toolWidget->addWidget(w);

    QHBoxLayout *layout = new QHBoxLayout(w);
    layout->setMargin(0);
    layout->addSpacing(10);
    layout->addWidget(OpenPagesManager::instance().openPagesComboBox());

    layout->addWidget(new QLabel(tr("Filtered by:")));
    m_filterComboBox = new QComboBox;
    m_filterComboBox->setMinimumContentsLength(20);
    layout->addWidget(m_filterComboBox);
    connect(m_filterComboBox, SIGNAL(activated(QString)), this,
        SLOT(filterDocumentation(QString)));
    connect(m_filterComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(updateSideBarSource()));

    m_closeButton = new QToolButton();
    m_closeButton->setIcon(QIcon(":/core/images/closebutton.png"));
    m_closeButton->setToolTip(tr("Close current Page"));
    connect(m_closeButton, SIGNAL(clicked()), &OpenPagesManager::instance(),
        SLOT(closeCurrentPage()));
    connect(&OpenPagesManager::instance(), SIGNAL(pagesChanged()), this,
        SLOT(updateCloseButton()));
    layout->addStretch();
    layout->addWidget(m_closeButton);

    return toolWidget;
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
        const QString &attrValue = m_idFromContext.mid(m_idFromContext
            .lastIndexOf(QChar(':')) + 1);
        if (attrValue.isEmpty())
            return;

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

            if (attrValue == name || name.startsWith(attrValue + QLatin1Char('-'))) {
                QWebElement parent = element.parent();
                m_styleProperty = parent.styleProperty(property,
                    QWebElement::ComputedStyle);
                parent.setStyleProperty(property, QLatin1String("yellow"));
            }
        }
        m_oldAttrValue = attrValue;
#endif
    }
}

void HelpPlugin::handleHelpRequest(const QUrl &url)
{
    if (HelpViewer::launchWithExternalApp(url))
        return;

    QString address = url.toString();
    if (!Core::HelpManager::instance()->findFile(url).isValid()) {
        if (address.startsWith(HelpViewer::NsNokia)
            || address.startsWith(HelpViewer::NsTrolltech)) {
                // local help not installed, resort to external web help
                QString urlPrefix = QLatin1String("http://doc.trolltech.com/");
                if (url.authority() == QLatin1String("com.nokia.qtcreator")) {
                    urlPrefix.append(QString::fromLatin1("qtcreator"));
                } else {
                    urlPrefix.append(QLatin1String("latest"));
                }
            address = urlPrefix + address.mid(address.lastIndexOf(QLatin1Char('/')));
        }
    }

    const QUrl newUrl(address);
    if (newUrl.queryItemValue(QLatin1String("view")) == QLatin1String("split")) {
        if (HelpViewer* viewer = viewerForContextMode())
            viewer->setSource(newUrl);
    } else {
        activateHelpMode();
        m_centralWidget->setSource(newUrl);
    }
}

Q_EXPORT_PLUGIN(HelpPlugin)
