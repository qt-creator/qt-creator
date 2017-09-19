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

#include <QClipboard>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QPlainTextEdit>
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

static HelpPlugin *m_instance = nullptr;

HelpPlugin::HelpPlugin()
{
    m_instance = this;
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
        const QString &creatorTrPath = ICore::resourcePath() + "/translations";
        const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString &trFile = QLatin1String("assistant_") + locale;
        const QString &helpTrFile = QLatin1String("qt_help_") + locale;
        if (qtr->load(trFile, qtTrPath) || qtr->load(trFile, creatorTrPath))
            QCoreApplication::installTranslator(qtr);
        if (qhelptr->load(helpTrFile, qtTrPath) || qhelptr->load(helpTrFile, creatorTrPath))
            QCoreApplication::installTranslator(qhelptr);
    }

    m_helpManager = new LocalHelpManager(this);
    m_openPagesManager = new OpenPagesManager(this);
    addAutoReleasedObject(m_docSettingsPage = new DocSettingsPage());
    addAutoReleasedObject(m_filterSettingsPage = new FilterSettingsPage());
    addAutoReleasedObject(new GeneralSettingsPage());
    addAutoReleasedObject(m_searchTaskHandler = new SearchTaskHandler);

    m_centralWidget = new CentralWidget(Context("Help.CentralHelpWidget"));
    connect(m_centralWidget, &HelpWidget::sourceChanged, this,
            &HelpPlugin::updateSideBarSource);
    connect(m_centralWidget, &CentralWidget::closeButtonClicked,
            &OpenPagesManager::instance(), &OpenPagesManager::closeCurrentPage);

    connect(LocalHelpManager::instance(), &LocalHelpManager::returnOnCloseChanged,
            m_centralWidget, &CentralWidget::updateCloseButton);
    connect(HelpManager::instance(), &HelpManager::helpRequested,
            this, &HelpPlugin::handleHelpRequest);
    connect(m_searchTaskHandler, &SearchTaskHandler::search, this, &QDesktopServices::openUrl);

    connect(m_filterSettingsPage, &FilterSettingsPage::filtersChanged, this,
        &HelpPlugin::setupHelpEngineIfNeeded);
    connect(HelpManager::instance(), &HelpManager::documentationChanged, this,
        &HelpPlugin::setupHelpEngineIfNeeded);
    connect(HelpManager::instance(), &HelpManager::collectionFileChanged, this,
        &HelpPlugin::setupHelpEngineIfNeeded);

    connect(ToolTip::instance(), &ToolTip::shown, ICore::instance(), []() {
        ICore::addAdditionalContext(Context(kToolTipHelpContext), ICore::ContextPriority::High);
    });
    connect(ToolTip::instance(), &ToolTip::hidden,ICore::instance(), []() {
        ICore::removeAdditionalContext(Context(kToolTipHelpContext));
    });

    Command *cmd;
    QAction *action;

    // Add Contents, Index, and Context menu items
    action = new QAction(QIcon::fromTheme("help-contents"),
        tr(Constants::SB_CONTENTS), this);
    cmd = ActionManager::registerAction(action, "Help.ContentsMenu");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, &QAction::triggered, this, &HelpPlugin::activateContents);

    action = new QAction(tr(Constants::SB_INDEX), this);
    cmd = ActionManager::registerAction(action, "Help.IndexMenu");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, &QAction::triggered, this, &HelpPlugin::activateIndex);

    action = new QAction(tr("Context Help"), this);
    cmd = ActionManager::registerAction(action, Help::Constants::CONTEXT_HELP,
                                        Context(kToolTipHelpContext, Core::Constants::C_GLOBAL));
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F1));
    connect(action, &QAction::triggered, this, &HelpPlugin::showContextHelp);

    action = new QAction(tr("Technical Support"), this);
    cmd = ActionManager::registerAction(action, "Help.TechSupport");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_SUPPORT);
    connect(action, &QAction::triggered, this, [this] {
        showLinkInHelpMode(QUrl("qthelp://org.qt-project.qtcreator/doc/technical-support.html"));
    });

    action = new QAction(tr("Report Bug..."), this);
    cmd = ActionManager::registerAction(action, "Help.ReportBug");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_SUPPORT);
    connect(action, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl("https://bugreports.qt.io"));
    });

    action = new QAction(tr("System Information..."), this);
    cmd = ActionManager::registerAction(action, "Help.SystemInformation");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_SUPPORT);
    connect(action, &QAction::triggered, this, &HelpPlugin::slotSystemInformation);

    if (ActionContainer *windowMenu = ActionManager::actionContainer(Core::Constants::M_WINDOW)) {
        // reuse EditorManager constants to avoid a second pair of menu actions
        // Goto Previous In History Action
        action = new QAction(this);
        Command *ctrlTab = ActionManager::registerAction(action, Core::Constants::GOTOPREVINHISTORY,
            modecontext);
        windowMenu->addAction(ctrlTab, Core::Constants::G_WINDOW_NAVIGATE);
        connect(action, &QAction::triggered, &OpenPagesManager::instance(),
            &OpenPagesManager::gotoPreviousPage);

        // Goto Next In History Action
        action = new QAction(this);
        Command *ctrlShiftTab = ActionManager::registerAction(action, Core::Constants::GOTONEXTINHISTORY,
            modecontext);
        windowMenu->addAction(ctrlShiftTab, Core::Constants::G_WINDOW_NAVIGATE);
        connect(action, &QAction::triggered, &OpenPagesManager::instance(),
            &OpenPagesManager::gotoNextPage);
    }

    auto helpIndexFilter = new HelpIndexFilter();
    addAutoReleasedObject(helpIndexFilter);
    connect(helpIndexFilter, &HelpIndexFilter::linksActivated,
            this, &HelpPlugin::showLinksInCurrentViewer);

    RemoteHelpFilter *remoteHelpFilter = new RemoteHelpFilter();
    addAutoReleasedObject(remoteHelpFilter);
    connect(remoteHelpFilter, &RemoteHelpFilter::linkActivated, this, &QDesktopServices::openUrl);

    QDesktopServices::setUrlHandler("qthelp", HelpManager::instance(), "handleHelpRequest");
    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &HelpPlugin::modeChanged);

    m_mode = new HelpMode;
    m_mode->setWidget(m_centralWidget);
    addAutoReleasedObject(m_mode);

    return true;
}

void HelpPlugin::extensionsInitialized()
{
    QStringList filesToRegister;
    // we might need to register creators inbuild help
    filesToRegister.append(ICore::documentationPath() + "/qtcreator.qch");
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
    QRegExp filterRegExp("Qt Creator \\d*\\.\\d*\\.\\d*");

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
    settings->setValue(kExternalWindowStateKey, qVariantFromValue(m_externalWindowState));
}

HelpWidget *HelpPlugin::createHelpWidget(const Context &context, HelpWidget::WidgetStyle style)
{
    HelpWidget *widget = new HelpWidget(context, style);

    connect(widget->currentViewer(), &HelpViewer::loadFinished,
            this, &HelpPlugin::highlightSearchTermsInContextHelp);
    connect(widget, &HelpWidget::openHelpMode,
            this, &HelpPlugin::showLinkInHelpMode);
    connect(widget, &HelpWidget::closeButtonClicked,
            this, &HelpPlugin::slotHideRightPane);
    connect(widget, &HelpWidget::aboutToClose,
            this, &HelpPlugin::saveExternalWindowSettings);

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
        m_externalWindowState = settings->value(kExternalWindowStateKey).toRect();
    }
    if (m_externalWindowState.isNull())
        m_externalWindow->resize(650, 700);
    else
        m_externalWindow->setGeometry(m_externalWindowState);
    m_externalWindow->show();
    return m_externalWindow->currentViewer();
}

HelpViewer *HelpPlugin::createHelpViewer(qreal zoom)
{
    // check for backends
    typedef std::function<HelpViewer *()> ViewerFactory;
    typedef QPair<QByteArray, ViewerFactory>  ViewerFactoryItem; // id -> factory
    QVector<ViewerFactoryItem> factories;
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
    showInHelpViewer(source, helpModeHelpViewer());
}

void HelpPlugin::showLinksInCurrentViewer(const QMap<QString, QUrl> &links, const QString &key)
{
    if (links.size() < 1)
        return;
    HelpWidget *widget = helpWidgetForWindow(QApplication::activeWindow());
    widget->showLinks(links, key);
}

void HelpPlugin::slotHideRightPane()
{
    RightPaneWidget::instance()->setShown(false);
}

void HelpPlugin::modeChanged(Core::Id mode, Core::Id old)
{
    Q_UNUSED(old)
    if (mode == m_mode->id()) {
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
        doSetupIfNeeded();
        QGuiApplication::restoreOverrideCursor();
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
    if (ModeManager::currentMode() == m_mode->id()
            || LocalHelpManager::contextHelpOption() == HelpManager::ExternalHelpAlways)
        LocalHelpManager::setupGuiHelpEngine();
}

bool HelpPlugin::canShowHelpSideBySide()
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

HelpViewer *HelpPlugin::helpModeHelpViewer()
{
    activateHelpMode(); // should trigger an createPage...
    HelpViewer *viewer = m_instance->m_centralWidget->currentViewer();
    if (!viewer)
        viewer = OpenPagesManager::instance().createPage();
    return viewer;
}

HelpWidget *HelpPlugin::helpWidgetForWindow(QWidget *window)
{
    if (m_externalWindow && m_externalWindow->window() == window->window())
        return m_externalWindow;
    activateHelpMode();
    return m_centralWidget;
}

HelpViewer *HelpPlugin::viewerForHelpViewerLocation(HelpManager::HelpViewerLocation location)
{
    HelpManager::HelpViewerLocation actualLocation = location;
    if (location == HelpManager::SideBySideIfPossible)
        actualLocation = canShowHelpSideBySide() ? HelpManager::SideBySideAlways
                                                 : HelpManager::HelpModeAlways;

    if (actualLocation == HelpManager::ExternalHelpAlways)
        return m_instance->externalHelpViewer();

    if (actualLocation == HelpManager::SideBySideAlways) {
        m_instance->createRightPaneContextViewer();
        RightPaneWidget::instance()->setWidget(m_instance->m_rightPaneSideBarWidget);
        RightPaneWidget::instance()->setShown(true);
        return m_instance->m_rightPaneSideBarWidget->currentViewer();
    }

    QTC_CHECK(actualLocation == HelpManager::HelpModeAlways);

    return m_instance->helpModeHelpViewer();
}

void HelpPlugin::showInHelpViewer(const QUrl &url, HelpViewer *viewer)
{
    QTC_ASSERT(viewer, return);
    viewer->setFocus();
    viewer->stop();
    viewer->setSource(url);
    ICore::raiseWindow(viewer);
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
    QRegExp exp("(\\d+)");
    foreach (const QUrl &link, links) {
        const QString &authority = link.authority();
        if (authority.startsWith("com.trolltech.")
                || authority.startsWith("org.qt-project.")) {
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
        showInHelpViewer(QUrl(Help::Constants::AboutBlank), viewer);
        viewer->setHtml(tr("<html><head><title>No Documentation</title>"
            "</head><body><br/><center>"
            "<font color=\"%1\"><b>%2</b></font><br/>"
            "<font color=\"%3\">No documentation available.</font>"
            "</center></body></html>")
            .arg(creatorTheme()->color(Theme::TextColorNormal).name())
            .arg(contextHelpId)
            .arg(creatorTheme()->color(Theme::TextColorNormal).name()));
    } else {
        showInHelpViewer(source, viewer);  // triggers loadFinished which triggers id highlighting
    }
}

void HelpPlugin::activateIndex()
{
    activateHelpMode();
    m_centralWidget->activateSideBarItem(Constants::HELP_INDEX);
}

void HelpPlugin::activateContents()
{
    activateHelpMode();
    m_centralWidget->activateSideBarItem(Constants::HELP_CONTENTS);
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
    static const QString qtcreatorUnversionedID = "org.qt-project.qtcreator";
    if (url.host() == qtcreatorUnversionedID) {
        // QtHelp doesn't know about versions, add the version number and use that
        QUrl versioned = url;
        versioned.setHost(qtcreatorUnversionedID + "."
                          + QString::fromLatin1(Core::Constants::IDE_VERSION_LONG).remove('.'));
        handleHelpRequest(versioned, location);
        return;
    }

    if (HelpViewer::launchWithExternalApp(url))
        return;

    if (!HelpManager::findFile(url).isValid()) {
        const QString address = url.toString();
        if (address.startsWith("qthelp://org.qt-project.")
                || address.startsWith("qthelp://com.nokia.")
                || address.startsWith("qthelp://com.trolltech.")) {
            // local help not installed, resort to external web help
            QString urlPrefix = "http://doc.qt.io/";
            if (url.authority().startsWith(qtcreatorUnversionedID))
                urlPrefix.append(QString::fromLatin1("qtcreator"));
            else
                urlPrefix.append("qt-5");
            QDesktopServices::openUrl(QUrl(urlPrefix + address.mid(address.lastIndexOf(QLatin1Char('/')))));
            return;
        }
    }

    HelpViewer *viewer = viewerForHelpViewerLocation(location);
    showInHelpViewer(url, viewer);
}

class DialogClosingOnEscape : public QDialog
{
public:
    DialogClosingOnEscape(QWidget *parent = 0) : QDialog(parent) {}
    bool event(QEvent *event)
    {
        if (event->type() == QEvent::ShortcutOverride) {
            QKeyEvent *ke = static_cast<QKeyEvent *>(event);
            if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
                ke->accept();
                return true;
            }
        }
        return QDialog::event(event);
    }
};

void HelpPlugin::slotSystemInformation()
{
    auto dialog = new DialogClosingOnEscape(ICore::dialogParent());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowFlags(Qt::Window);
    dialog->setModal(true);
    dialog->setWindowTitle(tr("System Information"));
    auto layout = new QVBoxLayout;
    dialog->setLayout(layout);
    auto intro = new QLabel(tr("Use the following to provide more detailed information about your system to bug reports:"));
    intro->setWordWrap(true);
    layout->addWidget(intro);
    const QString text = "{noformat}\n" + ICore::systemInformation() + "\n{noformat}";
    auto info = new QPlainTextEdit;
    QFont font = info->font();
    font.setFamily("Courier");
    font.setStyleHint(QFont::TypeWriter);
    info->setFont(font);
    info->setPlainText(text);
    layout->addWidget(info);
    auto buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Cancel);
    buttonBox->addButton(tr("Copy to Clipboard"), QDialogButtonBox::AcceptRole);
    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    connect(dialog, &QDialog::accepted, info, [info]() {
        if (QApplication::clipboard())
            QApplication::clipboard()->setText(info->toPlainText());
    });
    connect(dialog, &QDialog::rejected, dialog, [dialog]{ dialog->close(); });
    dialog->resize(700, 400);
    ICore::registerWindow(dialog, Context("Help.SystemInformation"));
    dialog->show();
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
