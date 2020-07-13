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
#include "docsettingspage.h"
#include "filtersettingspage.h"
#include "generalsettingspage.h"
#include "helpconstants.h"
#include "helpfindsupport.h"
#include "helpicons.h"
#include "helpindexfilter.h"
#include "helpmanager.h"
#include "helpmode.h"
#include "helpviewer.h"
#include "helpwidget.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"
#include "searchtaskhandler.h"
#include "searchwidget.h"
#include "topicchooser.h"

#include <bookmarkmanager.h>
#include <contentwindow.h>
#include <indexwindow.h>

#include <app/app_version.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/helpitem.h>
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
#include <QRegularExpression>

#include <QAction>
#include <QComboBox>
#include <QDesktopServices>
#include <QMenu>
#include <QStackedLayout>
#include <QSplitter>

#include <QHelpEngine>

#include <functional>

static const char kExternalWindowStateKey[] = "Help/ExternalWindowState";
static const char kToolTipHelpContext[] = "Help.ToolTip";

using namespace Core;
using namespace Utils;

namespace Help {
namespace Internal {

class HelpPluginPrivate : public QObject
{
public:
    HelpPluginPrivate();

    void modeChanged(Utils::Id mode, Utils::Id old);

    void requestContextHelp();
    void showContextHelp(const HelpItem &contextHelp);
    void activateIndex();
    void activateContents();

    void saveExternalWindowSettings();
    void showLinksInCurrentViewer(const QMultiMap<QString, QUrl> &links, const QString &key);

    void setupHelpEngineIfNeeded();

    HelpViewer *showHelpUrl(const QUrl &url, Core::HelpManager::HelpViewerLocation location);

    void slotSystemInformation();

#ifndef HELP_NEW_FILTER_ENGINE
    void resetFilter();
#endif

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
    FilterSettingsPage m_filterSettingsPage;
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

bool HelpPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error)
    dd = new HelpPluginPrivate;
    return true;
}

HelpPluginPrivate::HelpPluginPrivate()
{
    const QString &locale = ICore::userInterfaceLanguage();
    if (!locale.isEmpty()) {
        auto qtr = new QTranslator(this);
        auto qhelptr = new QTranslator(this);
        const QString &creatorTrPath = ICore::resourcePath() + "/translations";
        const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString &trFile = QLatin1String("assistant_") + locale;
        const QString &helpTrFile = QLatin1String("qt_help_") + locale;
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

    connect(&m_filterSettingsPage, &FilterSettingsPage::filtersChanged,
            this, &HelpPluginPrivate::setupHelpEngineIfNeeded);
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
    action = new QAction(QIcon::fromTheme("help-contents"),
                         HelpPlugin::tr(Constants::SB_CONTENTS), this);
    cmd = ActionManager::registerAction(action, "Help.ContentsMenu");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, &QAction::triggered, this, &HelpPluginPrivate::activateContents);

    action = new QAction(HelpPlugin::tr(Constants::SB_INDEX), this);
    cmd = ActionManager::registerAction(action, "Help.IndexMenu");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_HELP);
    connect(action, &QAction::triggered, this, &HelpPluginPrivate::activateIndex);

    action = new QAction(HelpPlugin::tr("Context Help"), this);
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

    action = new QAction(HelpPlugin::tr("Technical Support..."), this);
    cmd = ActionManager::registerAction(action, "Help.TechSupport");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_SUPPORT);
    connect(action, &QAction::triggered, this, [this] {
        showHelpUrl(QUrl("qthelp://org.qt-project.qtcreator/doc/technical-support.html"),
                    Core::HelpManager::HelpModeAlways);
    });

    action = new QAction(HelpPlugin::tr("Report Bug..."), this);
    cmd = ActionManager::registerAction(action, "Help.ReportBug");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_SUPPORT);
    connect(action, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl("https://bugreports.qt.io/secure/CreateIssue.jspa?pid=10512"));
    });

    action = new QAction(HelpPlugin::tr("System Information..."), this);
    cmd = ActionManager::registerAction(action, "Help.SystemInformation");
    ActionManager::actionContainer(Core::Constants::M_HELP)->addAction(cmd, Core::Constants::G_HELP_SUPPORT);
    connect(action, &QAction::triggered, this, &HelpPluginPrivate::slotSystemInformation);

    connect(&helpIndexFilter, &HelpIndexFilter::linksActivated,
            this, &HelpPluginPrivate::showLinksInCurrentViewer);

    QDesktopServices::setUrlHandler("qthelp", HelpManager::instance(), "showHelpUrl");
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
    HelpManager::setupHelpManager();
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

#ifndef HELP_NEW_FILTER_ENGINE

void HelpPluginPrivate::resetFilter()
{
    const QString &filterInternal = QString::fromLatin1("Qt Creator %1.%2.%3")
        .arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR).arg(IDE_VERSION_RELEASE);
    const QRegularExpression filterRegExp("^Qt Creator \\d*\\.\\d*\\.\\d*$");

    QHelpEngineCore *engine = &LocalHelpManager::helpEngine();
    const QStringList &filters = engine->customFilters();
    for (const QString &filter : filters) {
        if (filterRegExp.match(filter).hasMatch() && filter != filterInternal)
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
    const QString filterName = HelpPlugin::tr("Unfiltered");
    engine->removeCustomFilter(filterName);
    engine->addCustomFilter(filterName, QStringList());
    engine->setCustomValue(Help::Constants::WeAddedFilterKey, 1);
    engine->setCustomValue(Help::Constants::PreviousFilterNameKey, filterName);
    engine->setCurrentFilter(filterName);

    LocalHelpManager::updateFilterModel();
    connect(engine, &QHelpEngineCore::setupFinished,
            LocalHelpManager::instance(), &LocalHelpManager::updateFilterModel);
}

#endif

void HelpPluginPrivate::saveExternalWindowSettings()
{
    if (!m_externalWindow)
        return;
    m_externalWindowState = m_externalWindow->geometry();
    QSettings *settings = ICore::settings();
    settings->setValue(kExternalWindowStateKey, QVariant::fromValue(m_externalWindowState));
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
    const HelpViewerFactory factory = LocalHelpManager::viewerBackend();
    QTC_ASSERT(factory.create, return nullptr);
    HelpViewer *viewer = factory.create();

    // initialize font
    viewer->setViewerFont(LocalHelpManager::fallbackFont());
    connect(LocalHelpManager::instance(), &LocalHelpManager::fallbackFontChanged,
            viewer, &HelpViewer::setViewerFont);

    // initialize zoom
    viewer->setScale(zoom);
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

void HelpPluginPrivate::modeChanged(Utils::Id mode, Utils::Id old)
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
    const QVariant tipHelpValue = Utils::ToolTip::contextHelp();
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
                                .arg(HelpPlugin::tr("No Documentation"))
                                .arg(creatorTheme()->color(Theme::BackgroundColorNormal).name())
                                .arg(creatorTheme()->color(Theme::TextColorNormal).name())
                                .arg(contextHelp.helpIds().join(", "))
                                .arg(HelpPlugin::tr("No documentation available.")));
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
    m_centralWidget->activateSideBarItem(Constants::HELP_INDEX);
}

void HelpPluginPrivate::activateContents()
{
    activateHelpMode();
    m_centralWidget->activateSideBarItem(Constants::HELP_CONTENTS);
}

HelpViewer *HelpPluginPrivate::showHelpUrl(const QUrl &url, Core::HelpManager::HelpViewerLocation location)
{
    static const QString qtcreatorUnversionedID = "org.qt-project.qtcreator";
    if (url.host() == qtcreatorUnversionedID) {
        // QtHelp doesn't know about versions, add the version number and use that
        QUrl versioned = url;
        versioned.setHost(qtcreatorUnversionedID + "."
                          + QString::fromLatin1(Core::Constants::IDE_VERSION_LONG).remove('.'));

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
    dialog->setWindowTitle(HelpPlugin::tr("System Information"));
    auto layout = new QVBoxLayout;
    dialog->setLayout(layout);
    auto intro = new QLabel(HelpPlugin::tr("Use the following to provide more detailed information about your system to bug reports:"));
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
    buttonBox->addButton(HelpPlugin::tr("Copy to Clipboard"), QDialogButtonBox::AcceptRole);
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

void HelpPluginPrivate::doSetupIfNeeded()
{
    LocalHelpManager::setupGuiHelpEngine();
    if (m_setupNeeded) {
#ifndef HELP_NEW_FILTER_ENGINE
        resetFilter();
#endif
        m_setupNeeded = false;
        m_centralWidget->openPagesManager()->setupInitialPages();
        LocalHelpManager::bookmarkManager().setupBookmarkModels();
    }
}

} // Internal
} // Help
