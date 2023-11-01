// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "helpplugin.h"

#include "bookmarkmanager.h"
#include "docsettingspage.h"
#include "filtersettingspage.h"
#include "generalsettingspage.h"
#include "helpconstants.h"
#include "helpfindsupport.h"
#include "helpicons.h"
#include "helpindexfilter.h"
#include "helpmanager.h"
#include "helptr.h"
#include "helpviewer.h"
#include "helpwidget.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"
#include "searchtaskhandler.h"
#include "topicchooser.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/helpitem.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/sidebar.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/kitmanager.h>

#include <texteditor/texteditorconstants.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stringutils.h>
#include <utils/qtcsettings.h>
#include <utils/theme/theme.h>
#include <utils/tooltip/tooltip.h>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHelpEngine>
#include <QLabel>
#include <QLibraryInfo>
#include <QMenu>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSplitter>
#include <QStackedLayout>
#include <QTimer>
#include <QTranslator>

#include <functional>

using namespace Core;
using namespace Utils;

namespace Help::Internal {

const char kExternalWindowStateKey[] = "Help/ExternalWindowState";
const char kToolTipHelpContext[] = "Help.ToolTip";

class HelpMode : public IMode
{
public:
    HelpMode()
    {
        setObjectName("HelpMode");
        setContext(Core::Context(Constants::C_MODE_HELP));
        setIcon(Icon::modeIcon(Icons::MODE_HELP_CLASSIC,
                               Icons::MODE_HELP_FLAT, Icons::MODE_HELP_FLAT_ACTIVE));
        setDisplayName(Tr::tr("Help"));
        setPriority(Constants::P_MODE_HELP);
        setId(Constants::ID_MODE_HELP);
    }
};

class HelpPluginPrivate : public QObject
{
public:
    HelpPluginPrivate();

    void modeChanged(Id mode, Id old);

    void requestContextHelp();
    void showContextHelp(const HelpItem &contextHelp);
    void activateIndex();
    void activateContents();

    void saveExternalWindowSettings();
    void showLinksInCurrentViewer(const QMultiMap<QString, QUrl> &links, const QString &key);

    void setupHelpEngineIfNeeded();

    HelpViewer *showHelpUrl(const QUrl &url, Core::HelpManager::HelpViewerLocation location);

    void slotSystemInformation();

    static void activateHelpMode() { ModeManager::activateMode(Constants::ID_MODE_HELP); }
    static bool canShowHelpSideBySide();

    HelpViewer *viewerForContextHelp();
    HelpWidget *createHelpWidget(const Core::Context &context, HelpWidget::WidgetStyle style);
    void createRightPaneContextViewer();
    HelpViewer *externalHelpViewer();
    HelpViewer *helpModeHelpViewer();
    HelpWidget *helpWidgetForWindow(QWidget *window);
    HelpViewer *viewerForHelpViewerLocation(Core::HelpManager::HelpViewerLocation location);

    void showInHelpViewer(const QUrl &url, HelpViewer *viewer);
    void doSetupIfNeeded();

    HelpMode m_mode;
    HelpWidget *m_centralWidget = nullptr;
    HelpWidget *m_rightPaneSideBarWidget = nullptr;
    QPointer<HelpWidget> m_externalWindow;
    QRect m_externalWindowState;

    DocSettingsPage m_docSettingsPage;
    FilterSettingsPage m_filterSettingsPage{[this] {setupHelpEngineIfNeeded(); }};
    SearchTaskHandler m_searchTaskHandler;
    GeneralSettingsPage m_generalSettingsPage;

    bool m_setupNeeded = true;
    LocalHelpManager m_localHelpManager;

    HelpIndexFilter helpIndexFilter;
};

static HelpPluginPrivate *dd = nullptr;
static HelpManager *m_helpManager = nullptr;

HelpPlugin::HelpPlugin()
{
    m_helpManager = new HelpManager;
}

HelpPlugin::~HelpPlugin()
{
    delete dd;
    dd = nullptr;
    delete m_helpManager;
    m_helpManager = nullptr;
}

void HelpPlugin::showHelpUrl(const QUrl &url, Core::HelpManager::HelpViewerLocation location)
{
    dd->showHelpUrl(url, location);
}

void HelpPlugin::showLinksInCurrentViewer(const QMultiMap<QString, QUrl> &links, const QString &key)
{
    dd->showLinksInCurrentViewer(links, key);
}

void HelpPlugin::initialize()
{
    dd = new HelpPluginPrivate;
}

HelpPluginPrivate::HelpPluginPrivate()
{
    const QString locale = ICore::userInterfaceLanguage();
    if (!locale.isEmpty()) {
        auto qtr = new QTranslator(this);
        auto qhelptr = new QTranslator(this);
        const QString creatorTrPath = ICore::resourcePath("translations").toString();
        const QString qtTrPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
        const QString trFile = QLatin1String("assistant_") + locale;
        const QString helpTrFile = QLatin1String("qt_help_") + locale;
        if (qtr->load(trFile, qtTrPath) || qtr->load(trFile, creatorTrPath))
            QCoreApplication::installTranslator(qtr);
        if (qhelptr->load(helpTrFile, qtTrPath) || qhelptr->load(helpTrFile, creatorTrPath))
            QCoreApplication::installTranslator(qhelptr);
    }

    m_centralWidget = createHelpWidget(Context("Help.CentralHelpWidget"), HelpWidget::ModeWidget);
    connect(HelpManager::instance(), &HelpManager::helpRequested,
            this, &HelpPluginPrivate::showHelpUrl);
    connect(&m_searchTaskHandler, &SearchTaskHandler::search,
            this, &QDesktopServices::openUrl);

    connect(Core::HelpManager::Signals::instance(),
            &Core::HelpManager::Signals::documentationChanged,
            this,
            &HelpPluginPrivate::setupHelpEngineIfNeeded);
    connect(HelpManager::instance(), &HelpManager::collectionFileChanged,
            this, &HelpPluginPrivate::setupHelpEngineIfNeeded);

    connect(ToolTip::instance(), &ToolTip::shown, ICore::instance(), []() {
        ICore::addAdditionalContext(Context(kToolTipHelpContext), ICore::ContextPriority::High);
    });
    connect(ToolTip::instance(), &ToolTip::hidden,ICore::instance(), []() {
        ICore::removeAdditionalContext(Context(kToolTipHelpContext));
    });

    Command *cmd;
    QAction *action;

    // Add Contents, Index, and Context menu items
    action = new QAction(QIcon::fromTheme("help-contents"), Tr::tr(Constants::SB_CONTENTS), this);
    cmd = ActionManager::registerAction(action, "Help.ContentsMenu");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, &QAction::triggered, this, &HelpPluginPrivate::activateContents);

    action = new QAction(Tr::tr(Constants::SB_INDEX), this);
    cmd = ActionManager::registerAction(action, "Help.IndexMenu");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, &QAction::triggered, this, &HelpPluginPrivate::activateIndex);

    action = new QAction(Tr::tr("Context Help"), this);
    cmd = ActionManager::registerAction(action, Help::Constants::CONTEXT_HELP,
                                        Context(kToolTipHelpContext, Core::Constants::C_GLOBAL));
    cmd->setTouchBarIcon(Icons::MACOS_TOUCHBAR_HELP.icon());
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    ActionManager::actionContainer(Core::Constants::TOUCH_BAR)
        ->addAction(cmd, Core::Constants::G_TOUCHBAR_HELP);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F1));
    connect(action, &QAction::triggered, this, &HelpPluginPrivate::requestContextHelp);
    ActionContainer *textEditorContextMenu = ActionManager::actionContainer(
        TextEditor::Constants::M_STANDARDCONTEXTMENU);
    if (textEditorContextMenu) {
        textEditorContextMenu->insertGroup(TextEditor::Constants::G_BOM,
                                           Core::Constants::G_HELP);
        textEditorContextMenu->addSeparator(Core::Constants::G_HELP);
        textEditorContextMenu->addAction(cmd, Core::Constants::G_HELP);
    }

    action = new QAction(Tr::tr("Technical Support..."), this);
    cmd = ActionManager::registerAction(action, "Help.TechSupport");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_SUPPORT);
    connect(action, &QAction::triggered, this, [this] {
        showHelpUrl(QUrl("qthelp://org.qt-project.qtcreator/doc/technical-support.html"),
                    Core::HelpManager::HelpModeAlways);
    });

    const Key qdsStandaloneEntry = "QML/Designer/StandAloneMode"; //entry from designer settings
    const bool isDesigner = Core::ICore::settings()->value(qdsStandaloneEntry, false).toBool();

    action = new QAction(Tr::tr("Report Bug..."), this);
    cmd = ActionManager::registerAction(action, "Help.ReportBug");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_SUPPORT);
    connect(action, &QAction::triggered, this, [isDesigner] {
        const QUrl bugreportUrl = isDesigner ? QString("https://bugreports.qt.io/secure/CreateIssue.jspa?pid=11740") //QDS
                                             : QString("https://bugreports.qt.io/secure/CreateIssue.jspa?pid=10512"); //QtC
        QDesktopServices::openUrl(bugreportUrl);
    });

    action = new QAction(Tr::tr("System Information..."), this);
    cmd = ActionManager::registerAction(action, "Help.SystemInformation");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_SUPPORT);
    connect(action, &QAction::triggered, this, &HelpPluginPrivate::slotSystemInformation);

    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &HelpPluginPrivate::modeChanged);

    m_mode.setWidget(m_centralWidget);
}

void HelpPlugin::extensionsInitialized()
{
    QStringList filesToRegister;
    // we might need to register creators inbuild help
    filesToRegister.append(Core::HelpManager::documentationPath() + "/qtcreator.qch");
    filesToRegister.append(Core::HelpManager::documentationPath() + "/qtcreator-dev.qch");
    Core::HelpManager::registerDocumentation(filesToRegister);
}

bool HelpPlugin::delayedInitialize()
{
    if (ProjectExplorer::KitManager::isLoaded()) {
        HelpManager::setupHelpManager();
    } else {
        connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitsLoaded,
                this, &HelpManager::setupHelpManager);
    }
    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag HelpPlugin::aboutToShutdown()
{
    delete dd->m_externalWindow.data();

    delete dd->m_centralWidget;
    dd->m_centralWidget = nullptr;

    delete dd->m_rightPaneSideBarWidget;
    dd->m_rightPaneSideBarWidget = nullptr;

    return SynchronousShutdown;
}

void HelpPluginPrivate::saveExternalWindowSettings()
{
    if (!m_externalWindow)
        return;
    m_externalWindowState = m_externalWindow->geometry();
    ICore::settings()->setValueWithDefault(kExternalWindowStateKey,
                                           QVariant::fromValue(m_externalWindowState));
}

HelpWidget *HelpPluginPrivate::createHelpWidget(const Context &context, HelpWidget::WidgetStyle style)
{
    auto widget = new HelpWidget(context, style);

    connect(widget, &HelpWidget::requestShowHelpUrl, this, &HelpPluginPrivate::showHelpUrl);
    connect(LocalHelpManager::instance(),
            &LocalHelpManager::returnOnCloseChanged,
            widget,
            &HelpWidget::updateCloseButton);
    connect(widget, &HelpWidget::closeButtonClicked, this, [widget] {
        if (widget->widgetStyle() == HelpWidget::SideBarWidget)
            RightPaneWidget::instance()->setShown(false);
        else if (widget->viewerCount() == 1 && LocalHelpManager::returnOnClose())
            ModeManager::activateMode(Core::Constants::MODE_EDIT);
    });
    connect(widget, &HelpWidget::aboutToClose,
            this, &HelpPluginPrivate::saveExternalWindowSettings);

    return widget;
}

void HelpPluginPrivate::createRightPaneContextViewer()
{
    if (m_rightPaneSideBarWidget)
        return;
    m_rightPaneSideBarWidget = createHelpWidget(Context(Constants::C_HELP_SIDEBAR),
                                                HelpWidget::SideBarWidget);
}

HelpViewer *HelpPluginPrivate::externalHelpViewer()
{
    if (m_externalWindow)
        return m_externalWindow->currentViewer();
    doSetupIfNeeded();
    // Deletion for this widget is taken care of in HelpPlugin::aboutToShutdown().
    m_externalWindow = createHelpWidget(Context(Constants::C_HELP_EXTERNAL),
                                        HelpWidget::ExternalWindow);
    if (m_externalWindowState.isNull()) {
        QtcSettings *settings = ICore::settings();
        m_externalWindowState = settings->value(kExternalWindowStateKey).toRect();
    }
    if (m_externalWindowState.isNull())
        m_externalWindow->resize(650, 700);
    else
        m_externalWindow->setGeometry(m_externalWindowState);
    m_externalWindow->show();
    return m_externalWindow->currentViewer();
}

HelpViewer *HelpPlugin::createHelpViewer()
{
    const HelpViewerFactory factory = LocalHelpManager::viewerBackend();
    QTC_ASSERT(factory.create, return nullptr);
    HelpViewer *viewer = factory.create();

    // initialize font
    viewer->setViewerFont(LocalHelpManager::fallbackFont());
    connect(LocalHelpManager::instance(), &LocalHelpManager::fallbackFontChanged,
            viewer, &HelpViewer::setViewerFont);

    // initialize zoom
    viewer->setFontZoom(LocalHelpManager::fontZoom());
    connect(LocalHelpManager::instance(), &LocalHelpManager::fontZoomChanged,
            viewer, &HelpViewer::setFontZoom);

    // initialize antialias
    viewer->setAntialias(LocalHelpManager::antialias());
    connect(LocalHelpManager::instance(),
            &LocalHelpManager::antialiasChanged,
            viewer,
            &HelpViewer::setAntialias);

    viewer->setScrollWheelZoomingEnabled(LocalHelpManager::isScrollWheelZoomingEnabled());
    connect(LocalHelpManager::instance(), &LocalHelpManager::scrollWheelZoomingEnabledChanged,
            viewer, &HelpViewer::setScrollWheelZoomingEnabled);

    // add find support
    auto agg = new Aggregation::Aggregate;
    agg->add(viewer);
    agg->add(new HelpViewerFindSupport(viewer));

    return viewer;
}

HelpWidget *HelpPlugin::modeHelpWidget()
{
    return dd->m_centralWidget;
}

void HelpPluginPrivate::showLinksInCurrentViewer(const QMultiMap<QString, QUrl> &links, const QString &key)
{
    if (links.size() < 1)
        return;
    HelpWidget *widget = helpWidgetForWindow(QApplication::activeWindow());
    widget->showLinks(links, key);
}

void HelpPluginPrivate::modeChanged(Id mode, Id old)
{
    Q_UNUSED(old)
    if (mode == m_mode.id()) {
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
        doSetupIfNeeded();
        QGuiApplication::restoreOverrideCursor();
    }
}

void HelpPluginPrivate::setupHelpEngineIfNeeded()
{
    LocalHelpManager::setEngineNeedsUpdate();
    if (ModeManager::currentModeId() == m_mode.id()
            || LocalHelpManager::contextHelpOption() == Core::HelpManager::ExternalHelpAlways)
        LocalHelpManager::setupGuiHelpEngine();
}

bool HelpPluginPrivate::canShowHelpSideBySide()
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

HelpViewer *HelpPluginPrivate::helpModeHelpViewer()
{
    activateHelpMode(); // should trigger an createPage...
    HelpViewer *viewer = m_centralWidget->currentViewer();
    if (!viewer)
        viewer = m_centralWidget->openNewPage(QUrl(Help::Constants::AboutBlank));
    return viewer;
}

HelpWidget *HelpPluginPrivate::helpWidgetForWindow(QWidget *window)
{
    if (m_externalWindow && m_externalWindow->window() == window->window())
        return m_externalWindow;
    activateHelpMode();
    return m_centralWidget;
}

HelpViewer *HelpPluginPrivate::viewerForHelpViewerLocation(
    Core::HelpManager::HelpViewerLocation location)
{
    Core::HelpManager::HelpViewerLocation actualLocation = location;
    if (location == Core::HelpManager::SideBySideIfPossible)
        actualLocation = canShowHelpSideBySide() ? Core::HelpManager::SideBySideAlways
                                                 : Core::HelpManager::HelpModeAlways;

    // force setup, as we might have never switched to full help mode
    // thus the help engine might still run without collection file setup
    LocalHelpManager::setupGuiHelpEngine();
    if (actualLocation == Core::HelpManager::ExternalHelpAlways)
        return externalHelpViewer();

    if (actualLocation == Core::HelpManager::SideBySideAlways) {
        createRightPaneContextViewer();
        ModeManager::activateMode(Core::Constants::MODE_EDIT);
        RightPaneWidget::instance()->setWidget(m_rightPaneSideBarWidget);
        RightPaneWidget::instance()->setShown(true);
        return m_rightPaneSideBarWidget->currentViewer();
    }

    QTC_CHECK(actualLocation == Core::HelpManager::HelpModeAlways);

    return helpModeHelpViewer();
}

void HelpPluginPrivate::showInHelpViewer(const QUrl &url, HelpViewer *viewer)
{
    QTC_ASSERT(viewer, return);
    viewer->setFocus();
    viewer->stop();
    viewer->setSource(url);
    ICore::raiseWindow(viewer);
    // Show the parent top-level-widget in case it was closed previously.
    viewer->window()->show();
}

HelpViewer *HelpPluginPrivate::viewerForContextHelp()
{
    return viewerForHelpViewerLocation(LocalHelpManager::contextHelpOption());
}

void HelpPluginPrivate::requestContextHelp()
{
    // Find out what to show
    const QVariant tipHelpValue = ToolTip::contextHelp();
    const HelpItem tipHelp = tipHelpValue.canConvert<HelpItem>()
                                 ? tipHelpValue.value<HelpItem>()
                                 : HelpItem(tipHelpValue.toString());
    IContext *context = ICore::currentContextObject();
    if (tipHelp.isEmpty() && context)
        context->contextHelp([this](const HelpItem &item) { showContextHelp(item); });
    else
        showContextHelp(tipHelp);
}

void HelpPluginPrivate::showContextHelp(const HelpItem &contextHelp)
{
    const HelpItem::Links links = contextHelp.bestLinks();
    if (links.empty()) {
        // No link found or no context object
        HelpViewer *viewer = showHelpUrl(QUrl(Help::Constants::AboutBlank),
                                         LocalHelpManager::contextHelpOption());
        if (viewer) {
            viewer->setHtml(QString("<html><head><title>%1</title>"
                                    "</head><body bgcolor=\"%2\"><br/><center>"
                                    "<font color=\"%3\"><b>%4</b></font><br/>"
                                    "<font color=\"%3\">%5</font>"
                                    "</center></body></html>")
                                .arg(Tr::tr("No Documentation"))
                                .arg(creatorTheme()->color(Theme::BackgroundColorNormal).name())
                                .arg(creatorTheme()->color(Theme::TextColorNormal).name())
                                .arg(contextHelp.helpIds().join(", "))
                                .arg(Tr::tr("No documentation available.")));
        }
    } else if (links.size() == 1 && !contextHelp.isFuzzyMatch()) {
        showHelpUrl(links.front().second, LocalHelpManager::contextHelpOption());
    } else {
        QMultiMap<QString, QUrl> map;
        for (const HelpItem::Link &link : links)
            map.insert(link.first, link.second);
        auto tc = new TopicChooser(ICore::dialogParent(), contextHelp.keyword(), map);
        tc->setModal(true);
        connect(tc, &QDialog::accepted, this, [this, tc] {
            showHelpUrl(tc->link(), LocalHelpManager::contextHelpOption());
        });
        connect(tc, &QDialog::finished, tc, [tc] { tc->deleteLater(); });
        tc->show();
    }
}

void HelpPluginPrivate::activateIndex()
{
    activateHelpMode();
    showHelpUrl(LocalHelpManager::homePage(), Core::HelpManager::HelpModeAlways);
    m_centralWidget->activateSideBarItem(Constants::HELP_INDEX);
}

void HelpPluginPrivate::activateContents()
{
    activateHelpMode();
    showHelpUrl(LocalHelpManager::homePage(), Core::HelpManager::HelpModeAlways);
    m_centralWidget->activateSideBarItem(Constants::HELP_CONTENTS);
}

HelpViewer *HelpPluginPrivate::showHelpUrl(const QUrl &url, Core::HelpManager::HelpViewerLocation location)
{
    static const QString qtcreatorUnversionedID = "org.qt-project.qtcreator";
    if (url.host() == qtcreatorUnversionedID) {
        // QtHelp doesn't know about versions, add the version number and use that
        QUrl versioned = url;
        versioned.setHost(qtcreatorUnversionedID + "."
                          + QCoreApplication::applicationVersion().remove('.'));

        return showHelpUrl(versioned, location);
    }

    if (HelpViewer::launchWithExternalApp(url))
        return nullptr;

    if (!HelpManager::findFile(url).isValid()) {
        if (LocalHelpManager::openOnlineHelp(url))
            return nullptr;
    }

    HelpViewer *viewer = viewerForHelpViewerLocation(location);
    showInHelpViewer(url, viewer);
    return viewer;
}

class DialogClosingOnEscape : public QDialog
{
public:
    DialogClosingOnEscape(QWidget *parent = nullptr) : QDialog(parent) {}
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::ShortcutOverride) {
            auto ke = static_cast<QKeyEvent *>(event);
            if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
                ke->accept();
                return true;
            }
        }
        return QDialog::event(event);
    }
};

void HelpPluginPrivate::slotSystemInformation()
{
    auto dialog = new DialogClosingOnEscape(ICore::dialogParent());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(true);
    dialog->setWindowTitle(Tr::tr("System Information"));
    auto layout = new QVBoxLayout;
    dialog->setLayout(layout);
    auto intro = new QLabel(Tr::tr("Use the following to provide more detailed information about "
                                   "your system to bug reports:"));
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
    buttonBox->addButton(Tr::tr("Copy to Clipboard"), QDialogButtonBox::AcceptRole);
    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    connect(dialog, &QDialog::accepted, info, [info]() {
        setClipboardAndSelection(info->toPlainText());
    });
    connect(dialog, &QDialog::rejected, dialog, [dialog]{ dialog->close(); });
    dialog->resize(700, 400);
    ICore::registerWindow(dialog, Context("Help.SystemInformation"));
    dialog->show();
}

void HelpPluginPrivate::doSetupIfNeeded()
{
    LocalHelpManager::setupGuiHelpEngine();
    if (m_setupNeeded) {
        m_setupNeeded = false;
        m_centralWidget->openPagesManager()->setupInitialPages();
        LocalHelpManager::bookmarkManager().setupBookmarkModels();
    }
}

} // Help::Internal
