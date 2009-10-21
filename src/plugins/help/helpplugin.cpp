/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "helpfindsupport.h"
#include "helpindexfilter.h"
#include "helpmode.h"
#include "helpviewer.h"
#include "indexwindow.h"
#include "searchwidget.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/actionmanager/actionmanager.h>
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

#include <welcome/welcomemode.h>

#include <texteditor/texteditorconstants.h>

#include <utils/styledbar.h>

#include <QtCore/QDebug>
#include <QtCore/qplugin.h>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QResource>
#include <QtCore/QLibraryInfo>
#include <QtCore/QTranslator>
#include <QtGui/QAction>
#include <QtGui/QShortcut>
#include <QtGui/QSplitter>
#include <QtGui/QStyle>
#include <QtGui/QToolBar>
#include <QtGui/QComboBox>
#include <QtGui/QDesktopServices>
#include <QtGui/QMessageBox>
#include <QtHelp/QHelpEngine>

#ifndef QT_NO_WEBKIT
#include <QtGui/QApplication>
#else
#include <QtWebKit/QWebSettings>
#endif

using namespace Help;
using namespace Help::Internal;

HelpManager::HelpManager(Internal::HelpPlugin* plugin)
    : m_plugin(plugin)
{
}

void HelpManager::registerDocumentation(const QStringList &fileNames)
{
    bool needsSetup = false;
    {
        QHelpEngineCore hc(m_plugin->helpEngine()->collectionFile());
        if (!hc.setupData())
            qWarning() << "Could not initialize help engine:" << hc.error();
        foreach (const QString &fileName, fileNames) {
            if (!QFileInfo(fileName).exists())
                continue;
            const QString &nameSpace = QHelpEngineCore::namespaceName(fileName);
            if (!nameSpace.isEmpty()
                && !hc.registeredDocumentations().contains(nameSpace)) {
                if (hc.registerDocumentation(fileName))
                    needsSetup = true;
                else
                    qDebug() << "error registering" << fileName << hc.error();
            }
        }
    }
    if (needsSetup)
        m_plugin->helpEngine()->setupData();
}

void HelpManager::openHelpPage(const QString& url)
{
    m_plugin->handleHelpRequest(url);
}

void HelpManager::openContextHelpPage(const QString& url)
{
    m_plugin->openContextHelpPage(url);
}

HelpPlugin::HelpPlugin() :
    m_core(0),
    m_helpEngine(0),
    m_contextHelpEngine(0),
    m_contentWidget(0),
    m_indexWidget(0),
    m_centralWidget(0),
    m_helpViewerForSideBar(0),
    m_mode(0),
    m_shownLastPages(false),
    m_contentItem(0),
    m_indexItem(0),
    m_searchItem(0),
    m_bookmarkItem(0),
    m_rightPaneSideBar(0)
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

    QString locale = qApp->property("qtc_locale").toString();
    if (!locale.isEmpty()) {
        QTranslator *qtr = new QTranslator(this);
        QTranslator *qhelptr = new QTranslator(this);
        const QString &creatorTrPath =
                Core::ICore::instance()->resourcePath() + QLatin1String("/translations");
        const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString &trFile = QLatin1String("assistant_") + locale;
        const QString &helpTrFile = QLatin1String("qt_help_") + locale;
        if (qtr->load(trFile, qtTrPath) || qtr->load(trFile, creatorTrPath))
            qApp->installTranslator(qtr);
        if (qhelptr->load(helpTrFile, qtTrPath) || qhelptr->load(helpTrFile, creatorTrPath))
            qApp->installTranslator(qhelptr);
    }

#ifndef QT_NO_WEBKIT
    QWebSettings *webSettings = QWebSettings::globalSettings();
    const QFont applicationFont = QApplication::font();
    webSettings->setFontFamily(QWebSettings::StandardFont, applicationFont.family());
    //webSettings->setFontSize(QWebSettings::DefaultFontSize, applicationFont.pointSize());
#endif

    // FIXME shouldn't the help engine create the directory if it doesn't exist?
    QFileInfo fi(m_core->settings()->fileName());
    QDir directory(fi.absolutePath()+"/qtcreator");
    if (!directory.exists())
        directory.mkpath(directory.absolutePath());
    m_helpEngine = new QHelpEngine(directory.absolutePath() +
        QLatin1String("/helpcollection.qhc"), this);
    connect(m_helpEngine, SIGNAL(setupFinished()), this,
        SLOT(updateFilterComboBox()));

    addAutoReleasedObject(new HelpManager(this));

    m_filterSettingsPage = new FilterSettingsPage(m_helpEngine);
    addAutoReleasedObject(m_filterSettingsPage);

    m_docSettingsPage = new DocSettingsPage(m_helpEngine);
    addAutoReleasedObject(m_docSettingsPage);

    connect(m_docSettingsPage, SIGNAL(documentationAdded()),
        m_filterSettingsPage, SLOT(updateFilterPage()));
    connect(m_docSettingsPage, SIGNAL(dialogAccepted()), this,
        SLOT(checkForHelpChanges()));

    m_contentWidget = new ContentWindow(m_helpEngine);
    m_contentWidget->setWindowTitle(tr("Contents"));
    m_indexWidget = new IndexWindow(m_helpEngine);
    m_indexWidget->setWindowTitle(tr("Index"));
    m_searchWidget = new SearchWidget(m_helpEngine->searchEngine());
    m_searchWidget->setWindowTitle(tr("Search"));
    m_bookmarkManager = new BookmarkManager(m_helpEngine);
    m_bookmarkWidget = new BookmarkWidget(m_bookmarkManager, 0, false);
    m_bookmarkWidget->setWindowTitle(tr("Bookmarks"));
    connect(m_bookmarkWidget, SIGNAL(addBookmark()), this, SLOT(addBookmark()));

    Core::ActionManager *am = m_core->actionManager();
    Core::Command *cmd;

    // Add Home, Previous and Next actions (used in the toolbar)
    QAction *homeAction =
        new QAction(QIcon(QLatin1String(":/help/images/home.png")), tr("Home"),
        this);
    cmd = am->registerAction(homeAction, QLatin1String("Help.Home"), globalcontext);

    QAction *previousAction =
        new QAction(QIcon(QLatin1String(":/help/images/previous.png")),
        tr("Previous Page"), this);
    cmd = am->registerAction(previousAction, QLatin1String("Help.Previous"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence::Back);

    QAction *nextAction =
        new QAction(QIcon(QLatin1String(":/help/images/next.png")), tr("Next Page"),
        this);
    cmd = am->registerAction(nextAction, QLatin1String("Help.Next"), modecontext);
    cmd->setDefaultKeySequence(QKeySequence::Forward);

    QAction *addBookmarkAction =
        new QAction(QIcon(QLatin1String(":/help/images/bookmark.png")),
        tr("Add Bookmark"), this);
    cmd = am->registerAction(addBookmarkAction, QLatin1String("Help.AddBookmark"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::Key_M));

    // Add Index, Contents, and Context menu items and a separator to the Help menu
    QAction *indexAction = new QAction(tr("Index"), this);
    cmd = am->registerAction(indexAction, QLatin1String("Help.Index"),
        globalcontext);
    am->actionContainer(Core::Constants::M_HELP)->addAction(cmd,
        Core::Constants::G_HELP_HELP);

    QAction *contentsAction = new QAction(tr("Contents"), this);
    cmd = am->registerAction(contentsAction, QLatin1String("Help.Contents"),
        globalcontext);
    am->actionContainer(Core::Constants::M_HELP)->addAction(cmd,
        Core::Constants::G_HELP_HELP);

    QAction *searchAction = new QAction(tr("Search"), this);
    cmd = am->registerAction(searchAction, QLatin1String("Help.Search"),
        globalcontext);
    am->actionContainer(Core::Constants::M_HELP)->addAction(cmd,
        Core::Constants::G_HELP_HELP);

    QAction *contextAction = new QAction(tr("Context Help"), this);
    cmd = am->registerAction(contextAction, QLatin1String("Help.Context"),
        globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F1));
    am->actionContainer(Core::Constants::M_HELP)->addAction(cmd,
        Core::Constants::G_HELP_HELP);

#ifndef Q_WS_MAC
    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Help.Separator"), globalcontext);
    am->actionContainer(Core::Constants::M_HELP)->addAction(cmd,
        Core::Constants::G_HELP_HELP);
#endif

    m_centralWidget = new Help::Internal::CentralWidget(m_helpEngine);
    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(m_centralWidget);
    agg->add(new HelpFindSupport(m_centralWidget));
    QWidget *mainWidget = new QWidget;
    QVBoxLayout *mainWidgetLayout = new QVBoxLayout(mainWidget);
    mainWidgetLayout->setMargin(0);
    mainWidgetLayout->setSpacing(0);
    mainWidgetLayout->addWidget(createToolBar());
    mainWidgetLayout->addWidget(m_centralWidget);

    m_contentItem = new Core::SideBarItem(m_contentWidget);
    m_indexItem = new Core::SideBarItem(m_indexWidget);
    m_searchItem = new Core::SideBarItem(m_searchWidget);
    m_bookmarkItem = new Core::SideBarItem(m_bookmarkWidget);
    QList<Core::SideBarItem*> itemList;
    itemList << m_contentItem << m_indexItem << m_searchItem << m_bookmarkItem;
    m_sideBar = new Core::SideBar(itemList, QList<Core::SideBarItem*>() << m_indexItem);

    QSplitter *splitter = new Core::MiniSplitter;
    splitter->setOpaqueResize(false);
    splitter->addWidget(m_sideBar);
    splitter->addWidget(mainWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>() << 300 << 300);

    m_mode = new HelpMode(splitter, m_centralWidget);
    m_mode->setContext(QList<int>() << modecontext);
    addAutoReleasedObject(m_mode);

    QAction *printAction = new QAction(this);
    am->registerAction(printAction, Core::Constants::PRINT, modecontext);
    connect(printAction, SIGNAL(triggered()), m_centralWidget, SLOT(print()));

    QAction *copyAction = new QAction(this);
    cmd = am->registerAction(copyAction, Core::Constants::COPY, modecontext);
    connect(copyAction, SIGNAL(triggered()), m_centralWidget, SLOT(copy()));
    copyAction->setText(cmd->action()->text());
    copyAction->setIcon(cmd->action()->icon());

    QMap<QString, Core::Command*> shortcutMap;
    QShortcut *shortcut = new QShortcut(splitter);
    shortcut->setWhatsThis(tr("Activate Index in Help mode"));
    cmd = am->registerShortcut(shortcut, QLatin1String("Help.IndexShortcut"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::Key_I));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateIndex()));
    shortcutMap.insert(m_indexWidget->windowTitle(), cmd);

    shortcut = new QShortcut(splitter);
    shortcut->setWhatsThis(tr("Activate Contents in Help mode"));
    cmd = am->registerShortcut(shortcut, QLatin1String("Help.ContentsShortcut"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::Key_T));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateContents()));
    shortcutMap.insert(m_contentWidget->windowTitle(), cmd);

    shortcut = new QShortcut(splitter);
    shortcut->setWhatsThis(tr("Activate Search in Help mode"));
    cmd = am->registerShortcut(shortcut, QLatin1String("Help.SearchShortcut"),
        modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::Key_S));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateSearch()));
    shortcutMap.insert(m_searchWidget->windowTitle(), cmd);
    shortcutMap.insert(m_bookmarkWidget->windowTitle(), 0);

    m_sideBar->setShortcutMap(shortcutMap);

    connect(homeAction, SIGNAL(triggered()), m_centralWidget, SLOT(home()));
    connect(previousAction, SIGNAL(triggered()), m_centralWidget, SLOT(backward()));
    connect(nextAction, SIGNAL(triggered()), m_centralWidget, SLOT(forward()));
    connect(addBookmarkAction, SIGNAL(triggered()), this, SLOT(addBookmark()));
    connect(m_contentWidget, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));
    connect(m_indexWidget, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));
    connect(m_searchWidget, SIGNAL(requestShowLink(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));
    connect(m_searchWidget, SIGNAL(requestShowLinkInNewTab(QUrl)),
        m_centralWidget, SLOT(setSourceInNewTab(QUrl)));
    connect(m_bookmarkWidget, SIGNAL(requestShowLink(QUrl)), m_centralWidget,
        SLOT(setSource(const QUrl&)));

    connect(m_centralWidget, SIGNAL(backwardAvailable(bool)),
        previousAction, SLOT(setEnabled(bool)));
    connect(m_centralWidget, SIGNAL(forwardAvailable(bool)),
        nextAction, SLOT(setEnabled(bool)));
    connect(m_centralWidget, SIGNAL(addNewBookmark(QString, QString)), this,
        SLOT(addNewBookmark(QString, QString)));

    QList<QAction*> actionList;
    actionList << previousAction
        << nextAction
        << homeAction
#ifndef Q_WS_MAC
        << sep
#endif
        << copyAction;
    m_centralWidget->setGlobalActions(actionList);

    connect(contextAction, SIGNAL(triggered()), this, SLOT(activateContext()));
    connect(indexAction, SIGNAL(triggered()), this, SLOT(activateIndex()));
    connect(contentsAction, SIGNAL(triggered()), this, SLOT(activateContents()));
    connect(searchAction, SIGNAL(triggered()), this, SLOT(activateSearch()));

    connect(m_core->modeManager(), SIGNAL(currentModeChanged(Core::IMode*)),
        this, SLOT(modeChanged(Core::IMode*)));

    connect(m_contentWidget, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));
    connect(m_indexWidget, SIGNAL(linkActivated(QUrl)), m_centralWidget,
        SLOT(setSource(QUrl)));
    connect(m_indexWidget, SIGNAL(linksActivated(QMap<QString, QUrl>, QString)),
        m_centralWidget, SLOT(showTopicChooser(QMap<QString, QUrl>, QString)));

    HelpIndexFilter *helpIndexFilter = new HelpIndexFilter(this, m_helpEngine);
    addAutoReleasedObject(helpIndexFilter);
    connect(helpIndexFilter, SIGNAL(linkActivated(QUrl)), this,
        SLOT(switchToHelpMode(QUrl)));
    connect(helpIndexFilter, SIGNAL(linksActivated(QMap<QString, QUrl>, QString)),
        this, SLOT(switchToHelpMode(QMap<QString, QUrl>, QString)));

    previousAction->setEnabled(m_centralWidget->isBackwardAvailable());
    nextAction->setEnabled(m_centralWidget->isForwardAvailable());

    createRightPaneSideBar();

    QDesktopServices::setUrlHandler("qthelp", this, "handleHelpRequest");

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
        cmd = am->registerAction(a, QLatin1String("Help.ResetFontSize"),
            modecontext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+0")));
        connect(a, SIGNAL(triggered()), m_centralWidget, SLOT(resetZoom()));
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);
    }

    GeneralSettingsPage *generalSettings = new GeneralSettingsPage(m_helpEngine,
        m_centralWidget, m_bookmarkManager);
    addAutoReleasedObject(generalSettings);
    connect(generalSettings, SIGNAL(fontChanged()), this, SLOT(fontChanged()));

    return true;
}

QHelpEngine* HelpPlugin::helpEngine() const
{
    return m_helpEngine;
}

void HelpPlugin::createRightPaneSideBar()
{
    QAction *switchToHelpMode = new QAction("Go to Help Mode", this);
    m_rightPaneBackwardAction =
        new QAction(QIcon(QLatin1String(":/help/images/previous.png")),
        tr("Previous"), this);
    m_rightPaneForwardAction =
        new QAction(QIcon(QLatin1String(":/help/images/next.png")), tr("Next"),
        this);

    QToolBar *rightPaneToolBar = new QToolBar();
    rightPaneToolBar->addAction(switchToHelpMode);
    rightPaneToolBar->addAction(m_rightPaneBackwardAction);
    rightPaneToolBar->addAction(m_rightPaneForwardAction);

    connect(switchToHelpMode, SIGNAL(triggered()), this, SLOT(switchToHelpMode()));
    connect(m_rightPaneBackwardAction, SIGNAL(triggered()), this,
        SLOT(rightPaneBackward()));
    connect(m_rightPaneForwardAction, SIGNAL(triggered()), this,
        SLOT(rightPaneForward()));

    QToolButton *closeButton = new QToolButton();
    closeButton->setIcon(QIcon(":/core/images/closebutton.png"));

    // Dummy layout to align the close button to the right
    QHBoxLayout *hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(0);
    hboxLayout->setMargin(0);
    hboxLayout->addWidget(rightPaneToolBar);
    hboxLayout->addStretch(5);
    hboxLayout->addWidget(closeButton);
    Utils::StyledBar *w = new Utils::StyledBar;
    w->setLayout(hboxLayout);
    connect(closeButton, SIGNAL(clicked()), this, SLOT(slotHideRightPane()));

    m_rightPaneSideBar = new QWidget;
    QVBoxLayout *rightPaneLayout = new QVBoxLayout;
    rightPaneLayout->setMargin(0);
    rightPaneLayout->setSpacing(0);
    m_rightPaneSideBar->setLayout(rightPaneLayout);
    m_rightPaneSideBar->setFocusProxy(m_helpViewerForSideBar);
    addAutoReleasedObject(new Core::BaseRightPaneWidget(m_rightPaneSideBar));

    rightPaneLayout->addWidget(w);
    m_helpViewerForSideBar = new HelpViewer(m_helpEngine, 0);
    Aggregation::Aggregate *agg = new Aggregation::Aggregate();
    agg->add(m_helpViewerForSideBar);
    agg->add(new HelpViewerFindSupport(m_helpViewerForSideBar));
    rightPaneLayout->addWidget(m_helpViewerForSideBar);
    rightPaneLayout->addWidget(new Core::FindToolBarPlaceHolder(m_rightPaneSideBar));
#if defined(QT_NO_WEBKIT)
    QFont font = m_helpViewerForSideBar->font();
    font = qVariantValue<QFont>(m_helpEngine->customValue(QLatin1String("font"),
        font));
    m_helpViewerForSideBar->setFont(font);
#endif
    m_core->addContextObject(new Core::BaseContext(m_helpViewerForSideBar, QList<int>()
        << m_core->uniqueIDManager()->uniqueIdentifier(Constants::C_HELP_SIDEBAR),
        this));
    connect(m_centralWidget, SIGNAL(sourceChanged(QUrl)), this,
        SLOT(updateSideBarSource(QUrl)));
    connect(m_centralWidget, SIGNAL(currentViewerChanged()), this,
        SLOT(updateSideBarSource()));

    QAction *copyActionSideBar = new QAction(this);
    Core::Command *cmd = m_core->actionManager()->registerAction(copyActionSideBar,
        Core::Constants::COPY, QList<int>()
        << m_core->uniqueIDManager()->uniqueIdentifier(Constants::C_HELP_SIDEBAR));
    connect(copyActionSideBar, SIGNAL(triggered()), this, SLOT(copyFromSideBar()));
    copyActionSideBar->setText(cmd->action()->text());
    copyActionSideBar->setIcon(cmd->action()->icon());
}

void HelpPlugin::copyFromSideBar()
{
    m_helpViewerForSideBar->copy();
}

void HelpPlugin::rightPaneBackward()
{
    m_helpViewerForSideBar->backward();
}

void HelpPlugin::rightPaneForward()
{
    m_helpViewerForSideBar->forward();
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

void HelpPlugin::extensionsInitialized()
{
    m_sideBar->readSettings(m_core->settings());
    if (!m_helpEngine->setupData()) {
        qWarning() << "Could not initialize help engine: " << m_helpEngine->error();
        return;
    }

    bool needsSetup = false;
    bool assistantInternalDocRegistered = false;
    QStringList documentationToRemove;
    QStringList filtersToRemove;

    const QString &docInternal = QString("com.nokia.qtcreator.%1%2%3")
        .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR).arg(IDE_VERSION_RELEASE);
    const QString filterInternal = QString("Qt Creator %1.%2.%3")
        .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR).arg(IDE_VERSION_RELEASE);
    const QRegExp filterRegExp("Qt Creator \\d*\\.\\d*\\.\\d*");
    const QStringList &docs = m_helpEngine->registeredDocumentations();
    foreach (const QString &ns, docs) {
        if (ns == docInternal) {
            assistantInternalDocRegistered = true;
        } else if (ns.startsWith("com.nokia.qtcreator.")) {
            documentationToRemove << ns;
        }
    }
    foreach (const QString &filter, m_helpEngine->customFilters()) {
        if (filterRegExp.exactMatch(filter) && filter != filterInternal) {
            filtersToRemove << filter;
        }
    }

    //remove any qtcreator documentation that doesn't belong to current version
    if (!documentationToRemove.isEmpty() || !filtersToRemove.isEmpty() || !assistantInternalDocRegistered) {
        QHelpEngineCore hc(m_helpEngine->collectionFile());
        hc.setupData();
        foreach (const QString &ns, documentationToRemove) {
            if (hc.unregisterDocumentation(ns))
                needsSetup = true;
        }
        foreach (const QString &filter, filtersToRemove) {
            if (hc.removeCustomFilter(filter))
                needsSetup = true;
        }

        if (!assistantInternalDocRegistered) {
            const QString qchFileName =
                QDir::cleanPath(QCoreApplication::applicationDirPath()
#if defined(Q_OS_MAC)
                + QLatin1String("/../Resources/doc/qtcreator.qch"));
#else
                + QLatin1String("../../share/doc/qtcreator/qtcreator.qch"));
#endif
            if (!hc.registerDocumentation(qchFileName))
                qDebug() << hc.error();
            needsSetup = true;
        }

    }

    QLatin1String key("UnfilteredFilterInserted");
    int i = m_helpEngine->customValue(key).toInt();
    if (i != 1) {
        {
            QHelpEngineCore hc(m_helpEngine->collectionFile());
            hc.setupData();
            hc.addCustomFilter(tr("Unfiltered"), QStringList());
            hc.setCustomValue(key, 1);
        }
        bool blocked = m_helpEngine->blockSignals(true);
        m_helpEngine->setCurrentFilter(tr("Unfiltered"));
        m_helpEngine->blockSignals(blocked);
        needsSetup = true;
    }

    QString addedDocs = m_helpEngine->customValue(QLatin1String("AddedDocs")).toString();
    if (!addedDocs.isEmpty()) {
        QStringList documentationToAdd = addedDocs.split(";");
        foreach(QString item, documentationToAdd) {
            needsSetup = true;
            m_helpEngine->registerDocumentation(item);
        }
        m_helpEngine->removeCustomValue(QLatin1String("AddedDocs"));
    }

    if (needsSetup)
        m_helpEngine->setupData();

    updateFilterComboBox();
    m_bookmarkManager->setupBookmarkModels();

#if !defined(QT_NO_WEBKIT)
    QWebSettings* webSettings = QWebSettings::globalSettings();
    QFont font(webSettings->fontFamily(QWebSettings::StandardFont),
        webSettings->fontSize(QWebSettings::DefaultFontSize));

    font = qVariantValue<QFont>(m_helpEngine->customValue(QLatin1String("font"),
        font));
    
    webSettings->setFontFamily(QWebSettings::StandardFont, font.family());
    webSettings->setFontSize(QWebSettings::DefaultFontSize, font.pointSize());
#endif

    QUrl url = m_helpEngine->findFile(QString::fromLatin1("qthelp://com."
        "trolltech.qt.440/qdoc/index.html"));
    if (!url.isValid()) {
        url.setUrl(QString::fromLatin1("qthelp://com.nokia.qtcreator.%1%2/doc/"
            "index.html").arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR));
    }
    m_helpEngine->setCustomValue(QLatin1String("DefaultHomePage"), url.toString());
}

void HelpPlugin::shutdown()
{
    m_sideBar->saveSettings(m_core->settings());
    m_bookmarkManager->saveBookmarks();
    delete m_bookmarkManager;
}

void HelpPlugin::setIndexFilter(const QString &filter)
{
    m_indexWidget->setSearchLineEditText(filter);
}

QString HelpPlugin::indexFilter() const
{
    return m_indexWidget->searchLineEditText();
}

void HelpPlugin::modeChanged(Core::IMode *mode)
{
    if (mode == m_mode && !m_shownLastPages) {
        m_shownLastPages = true;
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);
        m_centralWidget->setLastShownPages();
        qApp->restoreOverrideCursor();
    }
}

void HelpPlugin::updateSideBarSource()
{
    const QUrl &url = m_centralWidget->currentSource();
    if (url.isValid())
        updateSideBarSource(url);
}

void HelpPlugin::updateSideBarSource(const QUrl &newUrl)
{
    if (m_helpViewerForSideBar)
        m_helpViewerForSideBar->setSource(newUrl);
}

void HelpPlugin::fontChanged()
{
#if defined(QT_NO_WEBKIT)
    QFont font = qApp->font();
    font = qVariantValue<QFont>(m_helpEngine->customValue(QLatin1String("font"),
        font));

    if (m_helpViewerForSideBar)
        m_helpViewerForSideBar->setFont(font);

    if (m_centralWidget) {
        int i = 0;
        while (HelpViewer* viewer = m_centralWidget->helpViewerAtIndex(i++)) {
            if (viewer->font() != font)
                viewer->setFont(font);
        }
    }
#endif
}

HelpViewer* HelpPlugin::viewerForContextMode()
{
    HelpViewer *viewer = 0;
    bool showSideBySide = false;

    switch (m_helpEngine->customValue(QLatin1String("ContextHelpOption"), 0).toInt())
    {
    case 0: // side by side if possible
        {
            if (Core::IEditor *editor = Core::EditorManager::instance()->currentEditor()) {
                if (editor->widget() && editor->widget()->isVisible() && editor->widget()->width() < 800 )
                    break;
            }
        }
        // fall through
    case 1: // side by side
        showSideBySide = true;
        break;
    default: // help mode
        break;
    }

    Core::RightPanePlaceHolder* placeHolder = Core::RightPanePlaceHolder::current();
    if (placeHolder && showSideBySide) {
        Core::RightPaneWidget::instance()->setShown(true);
        viewer = m_helpViewerForSideBar;
    } else {
        if (!m_centralWidget->currentHelpViewer())
            activateHelpMode();
        viewer = m_centralWidget->currentHelpViewer();
    }

    return viewer;
}

void HelpPlugin::activateContext()
{
    Core::RightPanePlaceHolder* placeHolder = Core::RightPanePlaceHolder::current();
    if (placeHolder && m_helpViewerForSideBar->hasFocus()) {
        switchToHelpMode();
        return;
    } else if (m_core->modeManager()->currentMode() == m_mode)
        return;

    QString id;
    QMap<QString, QUrl> links;

    // Find out what to show
    if (Core::IContext *context = m_core->currentContextObject()) {
        if (!m_contextHelpEngine) {
            m_contextHelpEngine =
                new QHelpEngineCore(m_helpEngine->collectionFile(), this);
            m_contextHelpEngine->setupData();
            m_contextHelpEngine->setCurrentFilter(tr("Unfiltered"));
        }

        id = context->contextHelpId();
        links = m_contextHelpEngine->linksForIdentifier(id);
    }

    HelpViewer* viewer = viewerForContextMode();

    if (viewer) {
        if (links.isEmpty()) {
            // No link found or no context object
            viewer->setHtml(tr("<html><head><title>No Documentation</title>"
                "</head><body><br/><center><b>%1</b><br/>No documentation "
                "available.</center></body></html>").arg(id));
            viewer->setSource(QUrl());
        } else {
            QUrl source = *links.begin();
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
    openHelpPage(QString::fromLatin1("qthelp://com.nokia.qtcreator.%1%2%3/doc/index.html")
                 .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR).arg(IDE_VERSION_RELEASE));
}

void HelpPlugin::activateSearch()
{
    activateHelpMode();
    m_sideBar->activateItem(m_searchItem);
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
    //int size = toolWidget->style()->pixelMetric(QStyle::PM_SmallIconSize);
    //toolWidget->setIconSize(QSize(size, size));
    toolWidget->setMovable(false);

    toolWidget->addSeparator();

    QWidget *w = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(w);
    layout->setMargin(0);
    layout->addSpacing(10);
    layout->addWidget(new QLabel(tr("Filtered by:")));
    m_filterComboBox = new QComboBox;
    m_filterComboBox->setMinimumContentsLength(20);
    connect(m_filterComboBox, SIGNAL(activated(QString)), this,
        SLOT(filterDocumentation(QString)));
    layout->addWidget(m_filterComboBox);
    toolWidget->addWidget(w);

    return toolWidget;
}

void HelpPlugin::updateFilterComboBox()
{
    QString curFilter = m_filterComboBox->currentText();
    if (curFilter.isEmpty())
        curFilter = m_helpEngine->currentFilter();
    m_filterComboBox->clear();
    m_filterComboBox->addItems(m_helpEngine->customFilters());
    int idx = m_filterComboBox->findText(curFilter);
    if (idx < 0)
        idx = 0;
    m_filterComboBox->setCurrentIndex(idx);
}

void HelpPlugin::checkForHelpChanges()
{
    bool changed = m_docSettingsPage->applyChanges();
    changed |= m_filterSettingsPage->applyChanges();
    if (changed)
        m_helpEngine->setupData();
}

void HelpPlugin::filterDocumentation(const QString &customFilter)
{
    m_helpEngine->setCurrentFilter(customFilter);
}

void HelpPlugin::addBookmark()
{
    addNewBookmark(m_centralWidget->currentTitle(),
        m_centralWidget->currentSource().toString());
}

void HelpPlugin::addNewBookmark(const QString &title, const QString &url)
{
    if (url.isEmpty() || url == QLatin1String("about:blank"))
        return;

    m_bookmarkManager->showBookmarkDialog(m_centralWidget, title, url);
}

void HelpPlugin::handleHelpRequest(const QUrl& url)
{
    if (url.queryItemValue("view") == QLatin1String("split"))
        openContextHelpPage(url.toString());
    else
        openHelpPage(url.toString());
}

void HelpPlugin::openHelpPage(const QString& url)
{
    if (m_helpEngine->findFile(url).isValid()) {
        activateHelpMode();
        m_centralWidget->setSource(url);
    } else {
        // local help not installed, resort to external web help
        QString urlPrefix;
        if (url.startsWith("qthelp://com.nokia.qtcreator")) {
            urlPrefix = QString::fromLatin1("http://doc.trolltech.com/qtcreator-%1.%2/")
                        .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR);
        } else {
            urlPrefix = QLatin1String("http://doc.trolltech.com/latest/");
        }
        QDesktopServices::openUrl(urlPrefix + url.mid(url.lastIndexOf('/') + 1));
    }
}

void HelpPlugin::openContextHelpPage(const QString &url)
{
    Core::ModeManager *modeManager = Core::ICore::instance()->modeManager();
    if (modeManager->currentMode() == modeManager->mode(Core::Constants::MODE_WELCOME))
        modeManager->activateMode(Core::Constants::MODE_EDIT);
    HelpViewer* viewer = viewerForContextMode();
    viewer->setSource(QUrl(url));
}

Q_EXPORT_PLUGIN(HelpPlugin)
