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

#include "helpplugin.h"

#include "bookmarkmanager.h"
#include "centralwidget.h"
#include "docsettingspage.h"
#include "filtersettingspage.h"
#include "generalsettingspage.h"
#include "helpconstants.h"
#include "helpfindsupport.h"
#include "helpindexfilter.h"
#include "helpmode.h"
#include "helpviewer.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"
#include "openpagesmodel.h"
#include "qtwebkithelpviewer.h"
#include "remotehelpfilter.h"
#include "searchwidget.h"
#include "searchtaskhandler.h"
#include "textbrowserhelpviewer.h"

#ifdef QTC_MAC_NATIVE_HELPVIEWER
#include "macwebkithelpviewer.h"
#endif
#ifdef QTC_WEBENGINE_HELPVIEWER
#include "webenginehelpviewer.h"
#endif

#include <bookmarkmanager.h>
#include <contentwindow.h>
#include <indexwindow.h>

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
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/sidebar.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/find/findplugin.h>
#include <texteditor/texteditorconstants.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/theme/theme.h>
#include <utils/tooltip/tooltip.h>

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
#include <QStackedLayout>
#include <QSplitter>

#include <QHelpEngine>

#include <functional>

using namespace Help::Internal;

static const char kExternalWindowStateKey[] = "Help/ExternalWindowState";
static const char kToolTipHelpContext[] = "Help.ToolTip";

using namespace Core;
using namespace Utils;

HelpPlugin::HelpPlugin()
    : m_mode(0),
    m_centralWidget(0),
    m_rightPaneSideBarWidget(0),
    m_setupNeeded(true),
    m_helpManager(0),
    m_openPagesManager(0)
{
}

HelpPlugin::~HelpPlugin()
{
    delete m_openPagesManager;
    delete m_helpManager;
}

bool HelpPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error)
    Context modecontext(Constants::C_MODE_HELP);

    const QString &locale = ICore::userInterfaceLanguage();
    if (!locale.isEmpty()) {
        QTranslator *qtr = new QTranslator(this);
        QTranslator *qhelptr = new QTranslator(this);
        const QString &creatorTrPath = ICore::resourcePath()
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
    addAutoReleasedObject(new GeneralSettingsPage());
    addAutoReleasedObject(m_searchTaskHandler = new SearchTaskHandler);

    m_centralWidget = new CentralWidget(modecontext);
    connect(m_centralWidget, SIGNAL(sourceChanged(QUrl)), this,
        SLOT(updateSideBarSource(QUrl)));
    connect(m_centralWidget, &CentralWidget::closeButtonClicked,
            &OpenPagesManager::instance(), &OpenPagesManager::closeCurrentPage);

    connect(LocalHelpManager::instance(), &LocalHelpManager::returnOnCloseChanged,
            m_centralWidget, &CentralWidget::updateCloseButton);
    connect(HelpManager::instance(), SIGNAL(helpRequested(QUrl,Core::HelpManager::HelpViewerLocation)),
            this, SLOT(handleHelpRequest(QUrl,Core::HelpManager::HelpViewerLocation)));
    connect(m_searchTaskHandler, SIGNAL(search(QUrl)), this,
            SLOT(showLinkInHelpMode(QUrl)));

    connect(m_filterSettingsPage, SIGNAL(filtersChanged()), this,
        SLOT(setupHelpEngineIfNeeded()));
    connect(HelpManager::instance(), SIGNAL(documentationChanged()), this,
        SLOT(setupHelpEngineIfNeeded()));
    connect(HelpManager::instance(), SIGNAL(collectionFileChanged()), this,
        SLOT(setupHelpEngineIfNeeded()));

    connect(ToolTip::instance(), &ToolTip::shown, ICore::instance(), []() {
        ICore::addAdditionalContext(Context(kToolTipHelpContext), ICore::ContextPriority::High);
    });
    connect(ToolTip::instance(), &ToolTip::hidden,ICore::instance(), []() {
        ICore::removeAdditionalContext(Context(kToolTipHelpContext));
    });

    Command *cmd;
    QAction *action;

    // Add Contents, Index, and Context menu items
    action = new QAction(QIcon::fromTheme(QLatin1String("help-contents")),
        tr(Constants::SB_CONTENTS), this);
    cmd = ActionManager::registerAction(action, "Help.ContentsMenu");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, SIGNAL(triggered()), this, SLOT(activateContents()));

    action = new QAction(tr(Constants::SB_INDEX), this);
    cmd = ActionManager::registerAction(action, "Help.IndexMenu");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, SIGNAL(triggered()), this, SLOT(activateIndex()));

    action = new QAction(tr("Context Help"), this);
    cmd = ActionManager::registerAction(action, Help::Constants::CONTEXT_HELP,
                                        Context(kToolTipHelpContext, Core::Constants::C_GLOBAL));
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F1));
    connect(action, SIGNAL(triggered()), this, SLOT(showContextHelp()));

    action = new QAction(tr("Technical Support"), this);
    cmd = ActionManager::registerAction(action, "Help.TechSupport");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_SUPPORT);
    connect(action, SIGNAL(triggered()), this, SLOT(slotOpenSupportPage()));

    action = new QAction(tr("Report Bug..."), this);
    cmd = ActionManager::registerAction(action, "Help.ReportBug");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_SUPPORT);
    connect(action, SIGNAL(triggered()), this, SLOT(slotReportBug()));

    if (ActionContainer *windowMenu = ActionManager::actionContainer(Core::Constants::M_WINDOW)) {
        // reuse EditorManager constants to avoid a second pair of menu actions
        // Goto Previous In History Action
        action = new QAction(this);
        Command *ctrlTab = ActionManager::registerAction(action, Core::Constants::GOTOPREVINHISTORY,
            modecontext);
        windowMenu->addAction(ctrlTab, Core::Constants::G_WINDOW_NAVIGATE);
        connect(action, SIGNAL(triggered()), &OpenPagesManager::instance(),
            SLOT(gotoPreviousPage()));

        // Goto Next In History Action
        action = new QAction(this);
        Command *ctrlShiftTab = ActionManager::registerAction(action, Core::Constants::GOTONEXTINHISTORY,
            modecontext);
        windowMenu->addAction(ctrlShiftTab, Core::Constants::G_WINDOW_NAVIGATE);
        connect(action, SIGNAL(triggered()), &OpenPagesManager::instance(),
            SLOT(gotoNextPage()));
    }

    auto helpIndexFilter = new HelpIndexFilter();
    addAutoReleasedObject(helpIndexFilter);
    connect(helpIndexFilter, &HelpIndexFilter::linkActivated,
            this, &HelpPlugin::showLinkInHelpMode);
    connect(helpIndexFilter, &HelpIndexFilter::linksActivated,
            this, &HelpPlugin::showLinksInHelpMode);

    RemoteHelpFilter *remoteHelpFilter = new RemoteHelpFilter();
    addAutoReleasedObject(remoteHelpFilter);
    connect(remoteHelpFilter, SIGNAL(linkActivated(QUrl)), this,
        SLOT(showLinkInHelpMode(QUrl)));

    QDesktopServices::setUrlHandler(QLatin1String("qthelp"), HelpManager::instance(), "handleHelpRequest");
    connect(ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*,Core::IMode*)),
            this, SLOT(modeChanged(Core::IMode*,Core::IMode*)));

    m_mode = new HelpMode;
    m_mode->setWidget(m_centralWidget);
    addAutoReleasedObject(m_mode);

    return true;
}

void HelpPlugin::extensionsInitialized()
{
    QStringList filesToRegister;
    // we might need to register creators inbuild help
    filesToRegister.append(ICore::documentationPath() + QLatin1String("/qtcreator.qch"));
    HelpManager::registerDocumentation(filesToRegister);
}

ExtensionSystem::IPlugin::ShutdownFlag HelpPlugin::aboutToShutdown()
{
    if (m_externalWindow)
        delete m_externalWindow.data();
    if (m_centralWidget)
        delete m_centralWidget;
    if (m_rightPaneSideBarWidget)
        delete m_rightPaneSideBarWidget;
    return SynchronousShutdown;
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

    LocalHelpManager::updateFilterModel();
    connect(engine, &QHelpEngineCore::setupFinished,
            LocalHelpManager::instance(), &LocalHelpManager::updateFilterModel);
}

void HelpPlugin::saveExternalWindowSettings()
{
    if (!m_externalWindow)
        return;
    m_externalWindowState = m_externalWindow->geometry();
    QSettings *settings = ICore::settings();
    settings->setValue(QLatin1String(kExternalWindowStateKey),
                       qVariantFromValue(m_externalWindowState));
}

HelpWidget *HelpPlugin::createHelpWidget(const Context &context, HelpWidget::WidgetStyle style)
{
    HelpWidget *widget = new HelpWidget(context, style);

    connect(widget->currentViewer(), SIGNAL(loadFinished()),
            this, SLOT(highlightSearchTermsInContextHelp()));
    connect(widget, SIGNAL(openHelpMode(QUrl)),
            this, SLOT(showLinkInHelpMode(QUrl)));
    connect(widget, SIGNAL(closeButtonClicked()),
            this, SLOT(slotHideRightPane()));
    connect(widget, SIGNAL(aboutToClose()),
            this, SLOT(saveExternalWindowSettings()));

    // force setup, as we might have never switched to full help mode
    // thus the help engine might still run without collection file setup
    LocalHelpManager::setupGuiHelpEngine();

    return widget;
}

void HelpPlugin::createRightPaneContextViewer()
{
    if (m_rightPaneSideBarWidget)
        return;
    m_rightPaneSideBarWidget = createHelpWidget(Context(Constants::C_HELP_SIDEBAR),
                                                HelpWidget::SideBarWidget);
}

HelpViewer *HelpPlugin::externalHelpViewer()
{
    if (m_externalWindow)
        return m_externalWindow->currentViewer();
    doSetupIfNeeded();
    m_externalWindow = createHelpWidget(Context(Constants::C_HELP_EXTERNAL),
                                        HelpWidget::ExternalWindow);
    if (m_externalWindowState.isNull()) {
        QSettings *settings = ICore::settings();
        m_externalWindowState = settings->value(QLatin1String(kExternalWindowStateKey)).toRect();
    }
    if (m_externalWindowState.isNull())
        m_externalWindow->resize(650, 700);
    else
        m_externalWindow->setGeometry(m_externalWindowState);
    m_externalWindow->show();
    m_externalWindow->setFocus();
    return m_externalWindow->currentViewer();
}

HelpViewer *HelpPlugin::createHelpViewer(qreal zoom)
{
    // check for backends
    typedef std::function<HelpViewer *()> ViewerFactory;
    typedef QPair<QByteArray, ViewerFactory>  ViewerFactoryItem; // id -> factory
    QVector<ViewerFactoryItem> factories;
#ifndef QT_NO_WEBKIT
    factories.append(qMakePair(QByteArray("qtwebkit"), []() { return new QtWebKitHelpViewer(); }));
#endif
#ifdef QTC_WEBENGINE_HELPVIEWER
    factories.append(qMakePair(QByteArray("qtwebengine"), []() { return new WebEngineHelpViewer(); }));
#endif
    factories.append(qMakePair(QByteArray("textbrowser"), []() { return new TextBrowserHelpViewer(); }));

#ifdef QTC_MAC_NATIVE_HELPVIEWER
    // default setting
#ifdef QTC_MAC_NATIVE_HELPVIEWER_DEFAULT
     factories.prepend(qMakePair(QByteArray("native"), []() { return new MacWebKitHelpViewer(); }));
#else
     factories.append(qMakePair(QByteArray("native"), []() { return new MacWebKitHelpViewer(); }));
#endif
#endif

    HelpViewer *viewer = nullptr;

    // check requested backend
    const QByteArray backend = qgetenv("QTC_HELPVIEWER_BACKEND");
    if (!backend.isEmpty()) {
        const int pos = Utils::indexOf(factories, [backend](const ViewerFactoryItem &item) {
            return backend == item.first;
        });
        if (pos == -1) {
            qWarning("Help viewer backend \"%s\" not found, using default.", backend.constData());
        } else {
            viewer  = factories.at(pos).second();
        }
    }

    if (!viewer)
        viewer = factories.first().second();
    QTC_ASSERT(viewer, return nullptr);

    // initialize font
    viewer->setViewerFont(LocalHelpManager::fallbackFont());
    connect(LocalHelpManager::instance(), &LocalHelpManager::fallbackFontChanged,
            viewer, &HelpViewer::setViewerFont);

    // initialize zoom
    viewer->setScale(zoom);

    // add find support
    Aggregation::Aggregate *agg = new Aggregation::Aggregate();
    agg->add(viewer);
    agg->add(new HelpViewerFindSupport(viewer));

    return viewer;
}

void HelpPlugin::activateHelpMode()
{
    ModeManager::activateMode(Id(Constants::ID_MODE_HELP));
}

void HelpPlugin::showLinkInHelpMode(const QUrl &source)
{
    activateHelpMode();
    ICore::raiseWindow(m_mode->widget());
    m_centralWidget->setSource(source);
    m_centralWidget->setFocus();
}

void HelpPlugin::showLinksInHelpMode(const QMap<QString, QUrl> &links, const QString &key)
{
    activateHelpMode();
    ICore::raiseWindow(m_mode->widget());
    m_centralWidget->showTopicChooser(links, key);
}

void HelpPlugin::slotHideRightPane()
{
    RightPaneWidget::instance()->setShown(false);
}

void HelpPlugin::modeChanged(IMode *mode, IMode *old)
{
    Q_UNUSED(old)
    if (mode == m_mode) {
        qApp->setOverrideCursor(Qt::WaitCursor);
        doSetupIfNeeded();
        qApp->restoreOverrideCursor();
    }
}

void HelpPlugin::updateSideBarSource()
{
    if (HelpViewer *viewer = m_centralWidget->currentViewer()) {
        const QUrl &url = viewer->source();
        if (url.isValid())
            updateSideBarSource(url);
    }
}

void HelpPlugin::updateSideBarSource(const QUrl &newUrl)
{
    if (m_rightPaneSideBarWidget) {
        // This is called when setSource on the central widget is called.
        // Avoid nested setSource calls (even of different help viewers) by scheduling the
        // sidebar viewer update on the event loop (QTCREATORBUG-12742)
        QMetaObject::invokeMethod(m_rightPaneSideBarWidget->currentViewer(), "setSource",
                                  Qt::QueuedConnection, Q_ARG(QUrl, newUrl));
    }
}

void HelpPlugin::setupHelpEngineIfNeeded()
{
    LocalHelpManager::setEngineNeedsUpdate();
    if (ModeManager::currentMode() == m_mode
            || LocalHelpManager::contextHelpOption() == HelpManager::ExternalHelpAlways)
        LocalHelpManager::setupGuiHelpEngine();
}

bool HelpPlugin::canShowHelpSideBySide() const
{
    RightPanePlaceHolder *placeHolder = RightPanePlaceHolder::current();
    if (!placeHolder)
        return false;
    if (placeHolder->isVisible())
        return true;

    IEditor *editor = EditorManager::currentEditor();
    if (!editor)
        return true;
    QTC_ASSERT(editor->widget(), return true);
    if (!editor->widget()->isVisible())
        return true;
    if (editor->widget()->width() < 800)
        return false;
    return true;
}

HelpViewer *HelpPlugin::viewerForHelpViewerLocation(HelpManager::HelpViewerLocation location)
{
    HelpManager::HelpViewerLocation actualLocation = location;
    if (location == HelpManager::SideBySideIfPossible)
        actualLocation = canShowHelpSideBySide() ? HelpManager::SideBySideAlways
                                                 : HelpManager::HelpModeAlways;

    if (actualLocation == HelpManager::ExternalHelpAlways)
        return externalHelpViewer();

    if (actualLocation == HelpManager::SideBySideAlways) {
        createRightPaneContextViewer();
        RightPaneWidget::instance()->setWidget(m_rightPaneSideBarWidget);
        RightPaneWidget::instance()->setShown(true);
        return m_rightPaneSideBarWidget->currentViewer();
    }

    QTC_CHECK(actualLocation == HelpManager::HelpModeAlways);

    activateHelpMode(); // should trigger an createPage...
    HelpViewer *viewer = m_centralWidget->currentViewer();
    if (!viewer)
        viewer = OpenPagesManager::instance().createPage();
    return viewer;
}

HelpViewer *HelpPlugin::viewerForContextHelp()
{
    return viewerForHelpViewerLocation(LocalHelpManager::contextHelpOption());
}

static QUrl findBestLink(const QMap<QString, QUrl> &links, QString *highlightId)
{
    if (highlightId)
        highlightId->clear();
    if (links.isEmpty())
        return QUrl();
    QUrl source = links.constBegin().value();
    // workaround to show the latest Qt version
    int version = 0;
    QRegExp exp(QLatin1String("(\\d+)"));
    foreach (const QUrl &link, links) {
        const QString &authority = link.authority();
        if (authority.startsWith(QLatin1String("com.trolltech."))
                || authority.startsWith(QLatin1String("org.qt-project."))) {
            if (exp.indexIn(authority) >= 0) {
                const int tmpVersion = exp.cap(1).toInt();
                if (tmpVersion > version) {
                    source = link;
                    version = tmpVersion;
                    if (highlightId)
                        *highlightId = source.fragment();
                }
            }
        }
    }
    return source;
}

void HelpPlugin::showContextHelp()
{
    // Find out what to show
    QString contextHelpId = Utils::ToolTip::contextHelpId();
    IContext *context = ICore::currentContextObject();
    if (contextHelpId.isEmpty() && context)
        contextHelpId = context->contextHelpId();

    // get the viewer after getting the help id,
    // because a new window might be opened and therefore focus be moved
    HelpViewer *viewer = viewerForContextHelp();
    QTC_ASSERT(viewer, return);

    QMap<QString, QUrl> links = HelpManager::linksForIdentifier(contextHelpId);
    // Maybe the id is already an URL
    if (links.isEmpty() && LocalHelpManager::isValidUrl(contextHelpId))
        links.insert(contextHelpId, contextHelpId);

    QUrl source = findBestLink(links, &m_contextHelpHighlightId);
    if (!source.isValid()) {
        // No link found or no context object
        viewer->setSource(QUrl(Help::Constants::AboutBlank));
        viewer->setHtml(tr("<html><head><title>No Documentation</title>"
            "</head><body><br/><center>"
            "<font color=\"%1\"><b>%2</b></font><br/>"
            "<font color=\"%3\">No documentation available.</font>"
            "</center></body></html>")
            .arg(creatorTheme()->color(Theme::TextColorNormal).name())
            .arg(contextHelpId)
            .arg(creatorTheme()->color(Theme::TextColorNormal).name()));
    } else {
        viewer->setFocus();
        viewer->stop();
        viewer->setSource(source); // triggers loadFinished which triggers id highlighting
        ICore::raiseWindow(viewer);
    }
}

void HelpPlugin::activateIndex()
{
    activateHelpMode();
    m_centralWidget->activateSideBarItem(QLatin1String(Constants::HELP_INDEX));
}

void HelpPlugin::activateContents()
{
    activateHelpMode();
    m_centralWidget->activateSideBarItem(QLatin1String(Constants::HELP_CONTENTS));
}

void HelpPlugin::highlightSearchTermsInContextHelp()
{
    if (m_contextHelpHighlightId.isEmpty())
        return;
    HelpViewer *viewer = viewerForContextHelp();
    QTC_ASSERT(viewer, return);
    viewer->highlightId(m_contextHelpHighlightId);
    m_contextHelpHighlightId.clear();
}

void HelpPlugin::handleHelpRequest(const QUrl &url, HelpManager::HelpViewerLocation location)
{
    if (HelpViewer::launchWithExternalApp(url))
        return;

    QString address = url.toString();
    if (!HelpManager::findFile(url).isValid()) {
        if (address.startsWith(QLatin1String("qthelp://org.qt-project."))
            || address.startsWith(QLatin1String("qthelp://com.nokia."))
            || address.startsWith(QLatin1String("qthelp://com.trolltech."))) {
                // local help not installed, resort to external web help
                QString urlPrefix = QLatin1String("http://doc.qt.io/");
                if (url.authority() == QLatin1String("org.qt-project.qtcreator"))
                    urlPrefix.append(QString::fromLatin1("qtcreator"));
                else
                    urlPrefix.append(QLatin1String("latest"));
            address = urlPrefix + address.mid(address.lastIndexOf(QLatin1Char('/')));
        }
    }

    const QUrl newUrl(address);
    HelpViewer *viewer = viewerForHelpViewerLocation(location);
    QTC_ASSERT(viewer, return);
    viewer->setSource(newUrl);
    ICore::raiseWindow(viewer);
}

void HelpPlugin::slotOpenSupportPage()
{
    showLinkInHelpMode(QUrl(QLatin1String("qthelp://org.qt-project.qtcreator/doc/technical-support.html")));
}

void HelpPlugin::slotReportBug()
{
    QDesktopServices::openUrl(QUrl(QLatin1String("https://bugreports.qt.io")));
}

void HelpPlugin::doSetupIfNeeded()
{
    LocalHelpManager::setupGuiHelpEngine();
    if (m_setupNeeded) {
        resetFilter();
        m_setupNeeded = false;
        OpenPagesManager::instance().setupInitialPages();
        LocalHelpManager::bookmarkManager().setupBookmarkModels();
    }
}
