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
#include <QtHelp/QHelpEngineCore>

#if defined(QT_NO_WEBKIT)
#   include <QtGui/QApplication>
#else
#   include <QtWebKit/QWebSettings>
#endif

using namespace Core::Constants;
using namespace Help;
using namespace Help::Internal;

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

    addAutoReleasedObject(m_helpManager = new HelpManager(this));
    addAutoReleasedObject(m_openPagesManager = new OpenPagesManager(this));

    addAutoReleasedObject(m_docSettingsPage = new DocSettingsPage());
    addAutoReleasedObject(m_filterSettingsPage = new FilterSettingsPage());
    addAutoReleasedObject(m_generalSettingsPage = new GeneralSettingsPage());

    connect(m_docSettingsPage, SIGNAL(documentationChanged()), m_filterSettingsPage,
        SLOT(updateFilterPage()));
    connect(m_generalSettingsPage, SIGNAL(fontChanged()), this,
        SLOT(fontChanged()));
    connect(m_helpManager, SIGNAL(helpRequested(QString)), this,
        SLOT(handleHelpRequest(QString)));
    connect(m_filterSettingsPage, SIGNAL(filtersChanged()), this,
        SLOT(setupHelpEngineIfNeeded()));
    connect(m_docSettingsPage, SIGNAL(documentationChanged()), this,
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

    // Add Index, Contents, and Context menu items and a separator to the Help menu
    action = new QAction(tr("Index"), this);
    cmd = am->registerAction(action, QLatin1String("Help.Index"), globalcontext);
    am->actionContainer(M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, SIGNAL(triggered()), this, SLOT(activateIndex()));

    action = new QAction(tr("Contents"), this);
    cmd = am->registerAction(action, QLatin1String("Help.Contents"), globalcontext);
    am->actionContainer(M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, SIGNAL(triggered()), this, SLOT(activateContents()));

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

    if (Core::ActionContainer *advancedMenu =
        am->actionContainer(Core::Constants::M_EDIT_ADVANCED)) {
        // reuse TextEditor constants to avoid a second pair of menu actions
        QAction *a = new QAction(tr("Increase Font Size"), this);
        cmd = am->registerAction(a, TextEditor::Constants::INCREASE_FONT_SIZE,
            modecontext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl++")));
        connect(a, SIGNAL(triggered()), m_centralWidget, SLOT(zoomIn()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);

        a = new QAction(tr("Decrease Font Size"), this);
        cmd = am->registerAction(a, TextEditor::Constants::DECREASE_FONT_SIZE,
            modecontext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+-")));
        connect(a, SIGNAL(triggered()), m_centralWidget, SLOT(zoomOut()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);

        a = new QAction(tr("Reset Font Size"), this);
        cmd = am->registerAction(a,  TextEditor::Constants::RESET_FONT_SIZE,
            modecontext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+0")));
        connect(a, SIGNAL(triggered()), m_centralWidget, SLOT(resetZoom()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);
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
    const QString &filterInternal = QString::fromLatin1("Qt Creator %1.%2.%3")
        .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR).arg(IDE_VERSION_RELEASE);
    const QRegExp filterRegExp(QLatin1String("Qt Creator \\d*\\.\\d*\\.\\d*"));

    QHelpEngineCore *engine = &m_helpManager->helpEngineCore();
    const QStringList &filters = engine->customFilters();
    foreach (const QString &filter, filters) {
        if (filterRegExp.exactMatch(filter) && filter != filterInternal)
            engine->removeCustomFilter(filter);
    }

    const QString &docInternal = QString::fromLatin1("com.nokia.qtcreator.%1%2%3")
        .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR).arg(IDE_VERSION_RELEASE);

    foreach (const QString &ns, engine->registeredDocumentations()) {
        if (ns.startsWith(QLatin1String("com.nokia.qtcreator."))
            && ns != docInternal)
            m_helpManager->unregisterDocumentation(QStringList() << ns);
    }

    QStringList filesToRegister;
    // Explicitly register qml.qch if located in creator directory. This is only
    // needed for the creator-qml package, were we want to ship the documentation
    // without a qt development version.
    const QString &appPath = QCoreApplication::applicationDirPath();
    filesToRegister.append(QDir::cleanPath(QDir::cleanPath(appPath
        + QLatin1String(DOCPATH "qml.qch"))));

    // we might need to register creators inbuild help
    filesToRegister.append(QDir::cleanPath(appPath
        + QLatin1String(DOCPATH "qtcreator.qch")));

    // this comes from the installer
    const QLatin1String key("AddedDocs");
    const QString &addedDocs = engine->customValue(key).toString();
    if (!addedDocs.isEmpty()) {
        engine->removeCustomValue(key);
        filesToRegister += addedDocs.split(QLatin1Char(';'));
    }

    updateFilterComboBox();
    m_helpManager->verifyDocumenation();
    m_helpManager->registerDocumentation(filesToRegister);

    const QString &url = QString::fromLatin1("qthelp://com.nokia.qtcreator."
        "%1%2%3/doc/index.html").arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR)
        .arg(IDE_VERSION_RELEASE);
    engine->setCustomValue(QLatin1String("DefaultHomePage"), url);
    connect(engine, SIGNAL(setupFinished()), this, SLOT(updateFilterComboBox()));
}

void HelpPlugin::shutdown()
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
    m_indexItem = new Core::SideBarItem(indexWindow);

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
    shortcutMap.insert(indexWindow->windowTitle(), cmd);

    ContentWindow *contentWindow = new ContentWindow();
    contentWindow->setWindowTitle(tr("Contents"));
    m_contentItem = new Core::SideBarItem(contentWindow);
    connect(contentWindow, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));

    shortcut = new QShortcut(m_splitter);
    shortcut->setWhatsThis(tr("Activate Contents in Help mode"));
    cmd = am->registerShortcut(shortcut, QLatin1String("Help.ContentsShortcut"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateContents()));
    shortcutMap.insert(contentWindow->windowTitle(), cmd);

    SearchWidget *searchWidget = new SearchWidget();
    searchWidget->setWindowTitle(tr("Search"));
    m_searchItem = new Core::SideBarItem(searchWidget);
    connect(searchWidget, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSourceFromSearch(QUrl)));

    // TODO: enable and find a proper keysequence as this is ambiguous
    // shortcut = new QShortcut(m_splitter);
    // shortcut->setWhatsThis(tr("Activate Search in Help mode"));
    // cmd = am->registerShortcut(shortcut, QLatin1String("Help.SearchShortcut"),
    //     modecontext);
    // cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::Key_S));
    // connect(shortcut, SIGNAL(activated()), this, SLOT(activateSearch()));
    // shortcutMap.insert(searchWidget->windowTitle(), cmd);

    BookmarkManager *manager = &HelpManager::bookmarkManager();
    BookmarkWidget *bookmarkWidget = new BookmarkWidget(manager, 0, false);
    bookmarkWidget->setWindowTitle(tr("Bookmarks"));
    m_bookmarkItem = new Core::SideBarItem(bookmarkWidget);
    connect(bookmarkWidget, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));

    // TODO: enable and find a proper keysequence as this is ambiguous
    // shortcut = new QShortcut(m_splitter);
    // shortcut->setWhatsThis(tr("Activate Bookmarks in Help mode"));
    // cmd = am->registerShortcut(shortcut, QLatin1String("Help.BookmarkShortcut"),
    //     modecontext);
    // cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::Key_B));
    // connect(shortcut, SIGNAL(activated()), this, SLOT(activateBookmarks()));
    // shortcutMap.insert(bookmarkWidget->windowTitle(), cmd);

    QWidget *openPagesWidget = OpenPagesManager::instance().openPagesWidget();
    openPagesWidget->setWindowTitle(tr("Open Pages"));
    m_openPagesItem = new Core::SideBarItem(openPagesWidget);

    shortcut = new QShortcut(m_splitter);
    shortcut->setWhatsThis(tr("Activate Open Pages in Help mode"));
    cmd = am->registerShortcut(shortcut, QLatin1String("Help.PagesShortcut"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateOpenPages()));
    shortcutMap.insert(openPagesWidget->windowTitle(), cmd);

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
    const QLatin1String weAddedFilterKey("UnfilteredFilterInserted");
    const QLatin1String previousFilterNameKey("UnfilteredFilterName");

    QHelpEngineCore *core = &m_helpManager->helpEngineCore();
    if (core->customValue(weAddedFilterKey).toInt() == 1) {
        // we added a filter at some point, remove previously added filter
        const QString &filter = core->customValue(previousFilterNameKey).toString();
        if (!filter.isEmpty())
            core->removeCustomFilter(filter);
    }

    // potentially remove a filter with new name
    const QString filterName = tr("Unfiltered");
    core->removeCustomFilter(filterName);
    core->addCustomFilter(filterName, QStringList());
    core->setCustomValue(weAddedFilterKey, 1);
    core->setCustomValue(previousFilterNameKey, filterName);
    (&m_helpManager->helpEngine())->setCurrentFilter(filterName);
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

#if defined(QT_NO_WEBKIT)
    m_helpViewerForSideBar->setFont(qVariantValue<QFont>(m_helpManager->helpEngineCore()
        .customValue(QLatin1String("font"), QApplication::font())));
#endif

    QAction *copy = new QAction(this);
    Core::Command *cmd = m_core->actionManager()->registerAction(copy, Core::Constants::COPY,
        QList<int>() << m_core->uniqueIDManager()->uniqueIdentifier(Constants::C_HELP_SIDEBAR));
    copy->setText(cmd->action()->text());
    copy->setIcon(cmd->action()->icon());

    connect(copy, SIGNAL(triggered()), m_helpViewerForSideBar, SLOT(copy()));
    connect(next, SIGNAL(triggered()), m_helpViewerForSideBar, SLOT(forward()));
    connect(previous, SIGNAL(triggered()), m_helpViewerForSideBar, SLOT(backward()));
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

        setupUi();
        resetFilter();
        m_helpManager->setupGuiHelpEngine();
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

    const QHelpEngineCore &engine = m_helpManager->helpEngineCore();
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
    if (Core::ICore::instance()->modeManager()->currentMode() == m_mode)
        m_helpManager->setupGuiHelpEngine();
}

HelpViewer* HelpPlugin::viewerForContextMode()
{
    using namespace Core;

    bool showSideBySide = false;
    const QHelpEngineCore &engine = m_helpManager->helpEngineCore();
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

    QString id;
    QMap<QString, QUrl> links;

    // Find out what to show
    if (IContext *context = m_core->currentContextObject()) {
        id = context->contextHelpId();
        links = m_helpManager->helpEngineCore().linksForIdentifier(id);
    }

    if (HelpViewer* viewer = viewerForContextMode()) {
        if (links.isEmpty()) {
            // No link found or no context object
            viewer->setHtml(tr("<html><head><title>No Documentation</title>"
                "</head><body><br/><center><b>%1</b><br/>No documentation "
                "available.</center></body></html>").arg(id));
            viewer->setSource(QUrl());
        } else {
            const QUrl &source = *links.begin();
            if (viewer->source() != source)
                viewer->setSource(source);
            viewer->setFocus();
        }
        if (viewer != m_helpViewerForSideBar)
            activateHelpMode();
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
    const QHelpEngine &engine = m_helpManager->helpEngine();
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
    (&m_helpManager->helpEngine())->setCurrentFilter(customFilter);
}

void HelpPlugin::addBookmark()
{
    HelpViewer *viewer = m_centralWidget->currentHelpViewer();

    const QString &url = viewer->source().toString();
    if (url.isEmpty() || url == Help::Constants::AboutBlank)
        return;

    BookmarkManager *manager = &HelpManager::bookmarkManager();
    manager->showBookmarkDialog(m_centralWidget, viewer->title(), url);
}

void HelpPlugin::handleHelpRequest(const QString &address)
{
    if (HelpViewer::launchWithExternalApp(address))
        return;

    if (m_helpManager->helpEngineCore().findFile(address).isValid()) {
        const QUrl url(address);
        if (url.queryItemValue(QLatin1String("view")) == QLatin1String("split")) {
            if (HelpViewer* viewer = viewerForContextMode())
                viewer->setSource(url);
        } else {
            activateHelpMode();
            m_centralWidget->setSource(url);
        }
    } else {
        // local help not installed, resort to external web help
        QString urlPrefix;
        if (address.startsWith(QLatin1String("qthelp://com.nokia.qtcreator"))) {
            urlPrefix = QString::fromLatin1("http://doc.trolltech.com/qtcreator"
                "-%1.%2/").arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR);
        } else {
            urlPrefix = QLatin1String("http://doc.trolltech.com/latest/");
        }
        QDesktopServices::openUrl(QUrl(urlPrefix + address.mid(address
            .lastIndexOf(QLatin1Char('/')) + 1)));
    }
}

Q_EXPORT_PLUGIN(HelpPlugin)
