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

#include "helpplugin.h"
#include "docsettingspage.h"
#include "filtersettingspage.h"
#include "helpindexfilter.h"
#include "helpmode.h"
#include "helpviewer.h"
#include "contentwindow.h"
#include "indexwindow.h"
#include "bookmarkmanager.h"
#include "centralwidget.h"
#include "helpfindsupport.h"
#include "searchwidget.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/sidebar.h>
#include <coreplugin/welcomemode.h>

#include <QtCore/QDebug>
#include <QtCore/qplugin.h>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QResource>
#include <QtGui/QAction>
#include <QtGui/QShortcut>
#include <QtGui/QSplitter>
#include <QtGui/QStyle>
#include <QtGui/QToolBar>
#include <QtGui/QComboBox>
#include <QtHelp/QHelpEngine>

using namespace Help;
using namespace Help::Internal;

HelpManager::HelpManager(QHelpEngine *helpEngine)
    : m_helpEngine(helpEngine)
{
}

void HelpManager::registerDocumentation(const QStringList &fileNames)
{
    bool needsSetup = false;
    {
        QHelpEngineCore hc(m_helpEngine->collectionFile());
        if (!hc.setupData())
            qWarning() << "Could not initialize help engine:" << hc.error();
        foreach (const QString &fileName, fileNames) {
            if (!QFile::exists(fileName))
                continue;
            QString fileNamespace = QHelpEngineCore::namespaceName(fileName);
            if (!fileNamespace.isEmpty() && !hc.registeredDocumentations().contains(fileNamespace)) {
                if (hc.registerDocumentation(fileName))
                    needsSetup = true;
                else
                    qDebug() << "error registering" << fileName << hc.error();
            }
        }
    }
    if (needsSetup)
        m_helpEngine->setupData();
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
    Q_UNUSED(arguments);
    Q_UNUSED(error);
    m_core = Core::ICore::instance();
    QList<int> globalcontext;
    globalcontext << Core::Constants::C_GLOBAL_ID;
    QList<int> modecontext;
    modecontext << m_core->uniqueIDManager()->uniqueIdentifier(Constants::C_MODE_HELP);

    // FIXME shouldn't the help engine create the directory if it doesn't exist?
    QFileInfo fi(m_core->settings()->fileName());
    QDir directory(fi.absolutePath()+"/qtcreator");
    if (!directory.exists())
        directory.mkpath(directory.absolutePath());
    m_helpEngine = new QHelpEngine(directory.absolutePath()
                                   + QLatin1String("/helpcollection.qhc"), this);
    connect(m_helpEngine, SIGNAL(setupFinished()),
        this, SLOT(updateFilterComboBox()));

    addAutoReleasedObject(new HelpManager(m_helpEngine));

    m_docSettingsPage = new DocSettingsPage(m_helpEngine);
    addAutoReleasedObject(m_docSettingsPage);

    m_filterSettingsPage = new FilterSettingsPage(m_helpEngine);
    addAutoReleasedObject(m_filterSettingsPage);
    connect(m_docSettingsPage, SIGNAL(documentationAdded()),
        m_filterSettingsPage, SLOT(updateFilterPage()));
    connect(m_docSettingsPage, SIGNAL(dialogAccepted()),
        this, SLOT(checkForHelpChanges()));

    m_contentWidget = new ContentWindow(m_helpEngine);
    m_contentWidget->setWindowTitle(tr("Contents"));
    m_indexWidget = new IndexWindow(m_helpEngine);
    m_indexWidget->setWindowTitle(tr("Index"));
    m_searchWidget = new SearchWidget(m_helpEngine->searchEngine());
    m_searchWidget->setWindowTitle(tr("Search"));
    m_bookmarkManager = new BookmarkManager(m_helpEngine);
    m_bookmarkWidget = new BookmarkWidget(m_bookmarkManager, 0, false);
    m_bookmarkWidget->setWindowTitle(tr("Bookmarks"));
    connect(m_bookmarkWidget, SIGNAL(addBookmark()),
        this, SLOT(addBookmark()));

    Core::ActionManager *am = m_core->actionManager();
    Core::Command *cmd;

    // Add Home, Previous and Next actions (used in the toolbar)
    QAction *homeAction = new QAction(QIcon(QLatin1String(":/help/images/home.png")), tr("Home"), this);
    cmd = am->registerAction(homeAction, QLatin1String("Help.Home"), globalcontext);

    QAction *previousAction = new QAction(QIcon(QLatin1String(":/help/images/previous.png")),
        tr("Previous"), this);
    cmd = am->registerAction(previousAction, QLatin1String("Help.Previous"), modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_Backspace));

    QAction *nextAction = new QAction(QIcon(QLatin1String(":/help/images/next.png")), tr("Next"), this);
    cmd = am->registerAction(nextAction, QLatin1String("Help.Next"), modecontext);

    QAction *addBookmarkAction = new QAction(QIcon(QLatin1String(":/help/images/bookmark.png")),
        tr("Add Bookmark"), this);
    cmd = am->registerAction(addBookmarkAction, QLatin1String("Help.AddBookmark"), modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::Key_M));

    // Add Index, Contents, and Context menu items and a separator to the Help menu
    QAction *indexAction = new QAction(tr("Index"), this);
    cmd = am->registerAction(indexAction, QLatin1String("Help.Index"), globalcontext);
    am->actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);

    QAction *contentsAction = new QAction(tr("Contents"), this);
    cmd = am->registerAction(contentsAction, QLatin1String("Help.Contents"), globalcontext);
    am->actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);

    QAction *searchAction = new QAction(tr("Search"), this);
    cmd = am->registerAction(searchAction, QLatin1String("Help.Search"), globalcontext);
    am->actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);

    QAction *contextAction = new QAction(tr("Context Help"), this);
    cmd = am->registerAction(contextAction, QLatin1String("Help.Context"), globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F1));
    am->actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);

#ifndef Q_OS_MAC
    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Help.Separator"), globalcontext);
    am->actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
#endif

    m_centralWidget = new CentralWidget(m_helpEngine);
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
    cmd = am->registerShortcut(shortcut, QLatin1String("Help.IndexShortcut"), modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::Key_I));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateIndex()));
    shortcutMap.insert(m_indexWidget->windowTitle(), cmd);

    shortcut = new QShortcut(splitter);
    shortcut->setWhatsThis(tr("Activate Contents in Help mode"));
    cmd = am->registerShortcut(shortcut, QLatin1String("Help.ContentsShortcut"), modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::Key_T));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateContents()));
    shortcutMap.insert(m_contentWidget->windowTitle(), cmd);

    shortcut = new QShortcut(splitter);
    shortcut->setWhatsThis(tr("Activate Search in Help mode"));
    cmd = am->registerShortcut(shortcut, QLatin1String("Help.SearchShortcut"), modecontext);
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::Key_S));
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateSearch()));
    shortcutMap.insert(m_searchWidget->windowTitle(), cmd);
    shortcutMap.insert(m_bookmarkWidget->windowTitle(), 0);

    m_sideBar->setShortcutMap(shortcutMap);

    connect(homeAction, SIGNAL(triggered()), m_centralWidget, SLOT(home()));
    connect(previousAction, SIGNAL(triggered()), m_centralWidget, SLOT(backward()));
    connect(nextAction, SIGNAL(triggered()), m_centralWidget, SLOT(forward()));
    connect(addBookmarkAction, SIGNAL(triggered()), this, SLOT(addBookmark()));
    connect(m_contentWidget, SIGNAL(linkActivated(const QUrl&)),
            m_centralWidget, SLOT(setSource(const QUrl&)));
    connect(m_indexWidget, SIGNAL(linkActivated(const QUrl&)),
            m_centralWidget, SLOT(setSource(const QUrl&)));
    connect(m_searchWidget, SIGNAL(requestShowLink(const QUrl&)),
        m_centralWidget, SLOT(setSource(const QUrl&)));
    connect(m_searchWidget, SIGNAL(requestShowLinkInNewTab(const QUrl&)),
        m_centralWidget, SLOT(setSourceInNewTab(const QUrl&)));
    connect(m_bookmarkWidget, SIGNAL(requestShowLink(const QUrl&)),
        m_centralWidget, SLOT(setSource(const QUrl&)));

    connect(m_centralWidget, SIGNAL(backwardAvailable(bool)),
        previousAction, SLOT(setEnabled(bool)));
    connect(m_centralWidget, SIGNAL(forwardAvailable(bool)),
        nextAction, SLOT(setEnabled(bool)));
    connect(m_centralWidget, SIGNAL(addNewBookmark(const QString&, const QString&)),
        this, SLOT(addNewBookmark(const QString&, const QString&)));

    QList<QAction*> actionList;
    actionList << previousAction
        << nextAction
        << homeAction
#ifndef Q_OS_MAC
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

    connect(m_contentWidget, SIGNAL(linkActivated(const QUrl&)),
        m_centralWidget, SLOT(setSource(const QUrl&)));
    connect(m_indexWidget, SIGNAL(linkActivated(const QUrl&)),
        m_centralWidget, SLOT(setSource(const QUrl&)));
    connect(m_indexWidget, SIGNAL(linksActivated(const QMap<QString, QUrl>&, const QString&)),
        m_centralWidget, SLOT(showTopicChooser(const QMap<QString, QUrl>&, const QString&)));

    HelpIndexFilter *helpIndexFilter = new HelpIndexFilter(this, m_helpEngine);
    addAutoReleasedObject(helpIndexFilter);
    connect(helpIndexFilter, SIGNAL(linkActivated(QUrl)),
            this, SLOT(switchToHelpMode(QUrl)));
    connect(helpIndexFilter, SIGNAL(linksActivated(const QMap<QString, QUrl>&, const QString&)),
            this, SLOT(switchToHelpMode(const QMap<QString, QUrl>&, const QString&)));

    previousAction->setEnabled(m_centralWidget->isBackwardAvailable());
    nextAction->setEnabled(m_centralWidget->isForwardAvailable());

    createRightPaneSideBar();

    return true;
}

void HelpPlugin::createRightPaneSideBar()
{
    QAction *switchToHelpMode = new QAction("Go to Help Mode", this);
    m_rightPaneBackwardAction = new QAction(QIcon(QLatin1String(":/help/images/previous.png")), tr("Previous"), this);
    m_rightPaneForwardAction = new QAction(QIcon(QLatin1String(":/help/images/next.png")), tr("Next"), this);

    QToolBar *rightPaneToolBar = new QToolBar();
    rightPaneToolBar->addAction(switchToHelpMode);
    rightPaneToolBar->addAction(m_rightPaneBackwardAction);
    rightPaneToolBar->addAction(m_rightPaneForwardAction);

    connect(switchToHelpMode, SIGNAL(triggered()), this, SLOT(switchToHelpMode()));
    connect(m_rightPaneBackwardAction, SIGNAL(triggered()), this, SLOT(rightPaneBackward()));
    connect(m_rightPaneForwardAction, SIGNAL(triggered()), this, SLOT(rightPaneForward()));

    QToolButton *closeButton = new QToolButton();
    closeButton->setProperty("type", QLatin1String("dockbutton"));
    closeButton->setIcon(QIcon(":/core/images/closebutton.png"));

    // Dummy layout to align the close button to the right
    QHBoxLayout *hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(0);
    hboxLayout->setMargin(0);
    hboxLayout->addStretch(5);
    hboxLayout->addWidget(closeButton);

    QWidget *w = new QWidget(rightPaneToolBar);
    w->setLayout(hboxLayout);
    rightPaneToolBar->addWidget(w);
    connect(closeButton, SIGNAL(clicked()), this, SLOT(slotHideRightPane()));

    QVBoxLayout *rightPaneLayout = new QVBoxLayout;
    rightPaneLayout->setMargin(0);
    rightPaneLayout->setSpacing(0);
    rightPaneLayout->addWidget(rightPaneToolBar);

    m_helpViewerForSideBar = new HelpViewer(m_helpEngine, 0);
    rightPaneLayout->addWidget(m_helpViewerForSideBar);

    m_rightPaneSideBar = new QWidget;
    m_rightPaneSideBar->setLayout(rightPaneLayout);
    m_rightPaneSideBar->setFocusProxy(m_helpViewerForSideBar);
    addAutoReleasedObject(new Core::BaseRightPaneWidget(m_rightPaneSideBar));
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
    Core::RightPaneWidget::instance()->setShown(false);
}

void HelpPlugin::switchToHelpMode(const QUrl &source)
{
    activateHelpMode();
    m_centralWidget->setSource(source);
    m_centralWidget->setFocus();
}

void HelpPlugin::switchToHelpMode(const QMap<QString, QUrl> &urls, const QString &keyword)
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

    foreach (QString ns, m_helpEngine->registeredDocumentations()) {
        if (ns == QString("com.nokia.qtcreator.%1%2")
            .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR)) {
            assistantInternalDocRegistered = true;
            break;
        }
    }

    if (!assistantInternalDocRegistered) {
        QFileInfo fi(m_helpEngine->collectionFile());

        const QString qchFileName =
            QDir::cleanPath(QCoreApplication::applicationDirPath()
#if defined(Q_OS_MAC)
            + QLatin1String("/../Resources/doc/qtcreator.qch"));
#else
            + QLatin1String("../../share/doc/qtcreator/qtcreator.qch"));
#endif
        QHelpEngineCore hc(fi.absoluteFilePath());
        hc.setupData();
        QString fileNamespace = QHelpEngineCore::namespaceName(qchFileName);
        if (!fileNamespace.isEmpty() && !hc.registeredDocumentations().contains(fileNamespace)) {
            if (!hc.registerDocumentation(qchFileName))
                qDebug() << hc.error();
            needsSetup = true;
        }
    }

    int i = m_helpEngine->customValue(
        QLatin1String("UnfilteredFilterInserted")).toInt();
    if (i != 1) {
        {
            QHelpEngineCore hc(m_helpEngine->collectionFile());
            hc.setupData();
            hc.addCustomFilter(tr("Unfiltered"), QStringList());
            hc.setCustomValue(QLatin1String("UnfilteredFilterInserted"), 1);
        }
        m_helpEngine->blockSignals(true);
        m_helpEngine->setCurrentFilter(tr("Unfiltered"));
        m_helpEngine->blockSignals(false);
        needsSetup = true;
    }

    if (needsSetup)
        m_helpEngine->setupData();

    updateFilterComboBox();
    m_bookmarkManager->setupBookmarkModels();

    if (Core::Internal::WelcomeMode *welcomeMode = qobject_cast<Core::Internal::WelcomeMode*>(m_core->modeManager()->mode(Core::Constants::MODE_WELCOME))) {
        connect(welcomeMode, SIGNAL(requestHelp(QString)), this, SLOT(openGettingStarted()));
    }
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

void HelpPlugin::activateContext()
{
    using namespace Core;
    // case 1 sidebar shown and has focus, we show whatever we have in the
    // sidebar in big
    RightPanePlaceHolder* placeHolder = RightPanePlaceHolder::current();
    if (placeHolder && Core::RightPaneWidget::instance()->hasFocus()) {
        switchToHelpMode();
        return;
    }

    bool useSideBar = false;
    if (placeHolder && !Core::RightPaneWidget::instance()->hasFocus())
        useSideBar = true;

    // Find out what to show
    HelpViewer *viewer = 0;
    if (IContext *context = m_core->currentContextObject()) {
        if (!m_contextHelpEngine) {
            m_contextHelpEngine = new QHelpEngineCore(m_helpEngine->collectionFile(), this);
            //m_contextHelpEngine->setAutoSaveFilter(false);
            m_contextHelpEngine->setupData();
            m_contextHelpEngine->setCurrentFilter(tr("Unfiltered"));
        }

        const QString &id = context->contextHelpId();
        QMap<QString, QUrl> links = m_contextHelpEngine->linksForIdentifier(id);
        if (!links.isEmpty()) {
            if (useSideBar) {
                Core::RightPaneWidget::instance()->setShown(true);
                viewer = m_helpViewerForSideBar;
            } else {
                viewer = m_centralWidget->currentHelpViewer();
                m_core->modeManager()->activateMode(QLatin1String(Constants::ID_MODE_HELP));
            }

            if (viewer) {
                QUrl source = *links.begin();
                if (viewer->source() != source)
                    viewer->setSource(source);
                viewer->setFocus();
            }
        } else {
            // No link found
            if (useSideBar) {
                Core::RightPaneWidget::instance()->setShown(true);
                viewer = m_helpViewerForSideBar;
            } else {
                viewer = m_centralWidget->currentHelpViewer();
                activateHelpMode();
            }
            
            if (viewer) {
                viewer->setHtml(tr("<html><head><title>No Documentation</title></head><body><br/>"
                    "<center><b>%1</b><br/>No documentation available.</center></body></html>").
                    arg(id));
                viewer->setSource(QUrl());
                //activateIndex();
            }
        }
    } else {
        // No context object
        if (useSideBar) {
            Core::RightPaneWidget::instance()->setShown(true);
            viewer = m_helpViewerForSideBar;
        } else {
            viewer = m_centralWidget->currentHelpViewer();
            activateHelpMode();
        }

        if (viewer) {
            viewer->setSource(QUrl());
            viewer->setHtml("<html><head><title>No Documentation</title></head><body><br/><br/><center>No"
                " documentation available.</center></body></html>");
            //activateIndex();
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
    connect(m_filterComboBox, SIGNAL(activated(const QString&)),
        this, SLOT(filterDocumentation(const QString&)));
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
    addNewBookmark(m_centralWidget->currentTitle(), m_centralWidget->currentSource().toString());
}

void HelpPlugin::addNewBookmark(const QString &title, const QString &url)
{
    if (url.isEmpty())
        return;

    m_bookmarkManager->showBookmarkDialog(m_centralWidget, title, url);
}

void HelpPlugin::openGettingStarted()
{
    activateHelpMode();
    m_centralWidget->setSource(
        QString("qthelp://com.nokia.qtcreator.%1%2/doc/index.html")
        .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR));
}


Q_EXPORT_PLUGIN(HelpPlugin)
