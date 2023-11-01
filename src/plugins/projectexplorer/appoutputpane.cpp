// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appoutputpane.h"

#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorericons.h"
#include "projectexplorertr.h"
#include "runcontrol.h"
#include "showoutputtaskhandler.h"
#include "windebuginterface.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/outputwindow.h>
#include <coreplugin/session.h>
#include <texteditor/behaviorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <extensionsystem/invoker.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QMenu>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

static Q_LOGGING_CATEGORY(appOutputLog, "qtc.projectexplorer.appoutput", QtWarningMsg);

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

const char OPTIONS_PAGE_ID[] = "B.ProjectExplorer.AppOutputOptions";
const char SETTINGS_KEY[] = "ProjectExplorer/AppOutput/Zoom";
const char C_APP_OUTPUT[] = "ProjectExplorer.ApplicationOutput";
const char POP_UP_FOR_RUN_OUTPUT_KEY[] = "ProjectExplorer/Settings/ShowRunOutput";
const char POP_UP_FOR_DEBUG_OUTPUT_KEY[] = "ProjectExplorer/Settings/ShowDebugOutput";
const char CLEAN_OLD_OUTPUT_KEY[] = "ProjectExplorer/Settings/CleanOldAppOutput";
const char MERGE_CHANNELS_KEY[] = "ProjectExplorer/Settings/MergeStdErrAndStdOut";
const char WRAP_OUTPUT_KEY[] = "ProjectExplorer/Settings/WrapAppOutput";
const char MAX_LINES_KEY[] = "ProjectExplorer/Settings/MaxAppOutputLines";

static QObject *debuggerPlugin()
{
    return ExtensionSystem::PluginManager::getObjectByName("DebuggerPlugin");
}

static QString msgAttachDebuggerTooltip(const QString &handleDescription = QString())
{
    return handleDescription.isEmpty() ?
           Tr::tr("Attach debugger to this process") :
           Tr::tr("Attach debugger to %1").arg(handleDescription);
}

class TabWidget : public QTabWidget
{
    Q_OBJECT
public:
    TabWidget(QWidget *parent = nullptr);
signals:
    void contextMenuRequested(const QPoint &pos, int index);
protected:
    bool eventFilter(QObject *object, QEvent *event) override;
private:
    void slotContextMenuRequested(const QPoint &pos);
    int m_tabIndexForMiddleClick = -1;
};

TabWidget::TabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    tabBar()->installEventFilter(this);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            this, &TabWidget::slotContextMenuRequested);
}

bool TabWidget::eventFilter(QObject *object, QEvent *event)
{
    if (object == tabBar()) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::MiddleButton) {
                m_tabIndexForMiddleClick = tabBar()->tabAt(me->pos());
                event->accept();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::MiddleButton) {
                int tab = tabBar()->tabAt(me->pos());
                if (tab != -1 && tab == m_tabIndexForMiddleClick)
                    emit tabCloseRequested(tab);
                m_tabIndexForMiddleClick = -1;
                event->accept();
                return true;
            }
        }
    }
    return QTabWidget::eventFilter(object, event);
}

void TabWidget::slotContextMenuRequested(const QPoint &pos)
{
    emit contextMenuRequested(pos, tabBar()->tabAt(pos));
}

AppOutputPane::RunControlTab::RunControlTab(RunControl *runControl, Core::OutputWindow *w) :
    runControl(runControl), window(w)
{
    if (runControl && w) {
        w->reset();
        runControl->setupFormatter(w->outputFormatter());
    }
}

AppOutputPane::AppOutputPane() :
    m_tabWidget(new TabWidget),
    m_stopAction(new QAction(Tr::tr("Stop"), this)),
    m_closeCurrentTabAction(new QAction(Tr::tr("Close Tab"), this)),
    m_closeAllTabsAction(new QAction(Tr::tr("Close All Tabs"), this)),
    m_closeOtherTabsAction(new QAction(Tr::tr("Close Other Tabs"), this)),
    m_reRunButton(new QToolButton),
    m_stopButton(new QToolButton),
    m_attachButton(new QToolButton),
    m_settingsButton(new QToolButton),
    m_formatterWidget(new QWidget),
    m_handler(new ShowOutputTaskHandler(this,
        Tr::tr("Show &App Output"),
        Tr::tr("Show the output that generated this issue in Application Output."),
        Tr::tr("A")))
{
    setId("ApplicationOutput");
    setDisplayName(Tr::tr("Application Output"));
    setPriorityInStatusBar(60);

    ExtensionSystem::PluginManager::addObject(m_handler);

    setObjectName("AppOutputPane"); // Used in valgrind engine
    loadSettings();

    // Rerun
    m_reRunButton->setIcon(Utils::Icons::RUN_SMALL_TOOLBAR.icon());
    m_reRunButton->setToolTip(Tr::tr("Re-run this run-configuration."));
    m_reRunButton->setEnabled(false);
    connect(m_reRunButton, &QToolButton::clicked,
            this, &AppOutputPane::reRunRunControl);

    // Stop
    m_stopAction->setIcon(Utils::Icons::STOP_SMALL_TOOLBAR.icon());
    m_stopAction->setToolTip(Tr::tr("Stop running program."));
    m_stopAction->setEnabled(false);

    Core::Command *cmd = Core::ActionManager::registerAction(m_stopAction, Constants::STOP);
    cmd->setDescription(m_stopAction->toolTip());

    m_stopButton->setDefaultAction(cmd->action());

    connect(m_stopAction, &QAction::triggered,
            this, &AppOutputPane::stopRunControl);

    // Attach
    m_attachButton->setToolTip(msgAttachDebuggerTooltip());
    m_attachButton->setEnabled(false);
    m_attachButton->setIcon(Icons::DEBUG_START_SMALL_TOOLBAR.icon());

    connect(m_attachButton, &QToolButton::clicked,
            this, &AppOutputPane::attachToRunControl);

    connect(this, &IOutputPane::zoomInRequested, this, &AppOutputPane::zoomIn);
    connect(this, &IOutputPane::zoomOutRequested, this, &AppOutputPane::zoomOut);
    connect(this, &IOutputPane::resetZoomRequested, this, &AppOutputPane::resetZoom);

    m_settingsButton->setToolTip(Core::ICore::msgShowOptionsDialog());
    m_settingsButton->setIcon(Utils::Icons::SETTINGS_TOOLBAR.icon());
    connect(m_settingsButton, &QToolButton::clicked, this, [] {
        Core::ICore::showOptionsDialog(OPTIONS_PAGE_ID);
    });

    auto formatterWidgetsLayout = new QHBoxLayout;
    formatterWidgetsLayout->setContentsMargins(QMargins());
    m_formatterWidget->setLayout(formatterWidgetsLayout);

    // Spacer (?)

    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, [this](int index) { closeTab(index); });

    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &AppOutputPane::tabChanged);
    connect(m_tabWidget, &TabWidget::contextMenuRequested,
            this, &AppOutputPane::contextMenuRequested);

    connect(SessionManager::instance(), &SessionManager::aboutToUnloadSession,
            this, &AppOutputPane::aboutToUnloadSession);

    setupFilterUi("AppOutputPane.Filter");
    setFilteringEnabled(false);
    setZoomButtonsEnabled(false);
    setupContext("Core.AppOutputPane", m_tabWidget);
}

AppOutputPane::~AppOutputPane()
{
    qCDebug(appOutputLog) << "AppOutputPane::~AppOutputPane: Entries left" << m_runControlTabs.size();

    for (const RunControlTab &rt : std::as_const(m_runControlTabs)) {
        delete rt.window;
        delete rt.runControl;
    }
    delete m_tabWidget;
    ExtensionSystem::PluginManager::removeObject(m_handler);
    delete m_handler;
}

AppOutputPane::RunControlTab *AppOutputPane::currentTab()
{
    return tabFor(m_tabWidget->currentWidget());
}

const AppOutputPane::RunControlTab *AppOutputPane::currentTab() const
{
    return tabFor(m_tabWidget->currentWidget());
}

RunControl *AppOutputPane::currentRunControl() const
{
    if (const RunControlTab * const tab = currentTab())
        return tab->runControl;
    return nullptr;
}

AppOutputPane::RunControlTab *AppOutputPane::tabFor(const RunControl *rc)
{
    const auto it = std::find_if(m_runControlTabs.begin(), m_runControlTabs.end(),
                                 [rc](RunControlTab &t) { return t.runControl == rc; });
    if (it == m_runControlTabs.end())
        return nullptr;
    return &*it;
}

AppOutputPane::RunControlTab *AppOutputPane::tabFor(const QWidget *outputWindow)
{
    const auto it = std::find_if(m_runControlTabs.begin(), m_runControlTabs.end(),
            [outputWindow](RunControlTab &t) { return t.window == outputWindow; });
    if (it == m_runControlTabs.end())
        return nullptr;
    return &*it;
}

const AppOutputPane::RunControlTab *AppOutputPane::tabFor(const QWidget *outputWindow) const
{
    return const_cast<AppOutputPane *>(this)->tabFor(outputWindow);
}

void AppOutputPane::updateCloseActions()
{
    const int tabCount = m_tabWidget->count();
    m_closeCurrentTabAction->setEnabled(tabCount > 0);
    m_closeAllTabsAction->setEnabled(tabCount > 0);
    m_closeOtherTabsAction->setEnabled(tabCount > 1);
}

bool AppOutputPane::aboutToClose() const
{
    return Utils::allOf(m_runControlTabs, [](const RunControlTab &rt) {
        return !rt.runControl || !rt.runControl->isRunning() || rt.runControl->promptToStop();
    });
}

void AppOutputPane::aboutToUnloadSession()
{
    closeTabs(CloseTabWithPrompt);
}

QWidget *AppOutputPane::outputWidget(QWidget *)
{
    return m_tabWidget;
}

QList<QWidget *> AppOutputPane::toolBarWidgets() const
{
    return QList<QWidget *>{m_reRunButton, m_stopButton, m_attachButton, m_settingsButton,
                m_formatterWidget} + IOutputPane::toolBarWidgets();
}

void AppOutputPane::clearContents()
{
    auto *currentWindow = qobject_cast<Core::OutputWindow *>(m_tabWidget->currentWidget());
    if (currentWindow)
        currentWindow->clear();
}

bool AppOutputPane::hasFocus() const
{
    QWidget *widget = m_tabWidget->currentWidget();
    if (!widget)
        return false;
    return widget->window()->focusWidget() == widget;
}

bool AppOutputPane::canFocus() const
{
    return m_tabWidget->currentWidget();
}

void AppOutputPane::setFocus()
{
    if (m_tabWidget->currentWidget())
        m_tabWidget->currentWidget()->setFocus();
}

void AppOutputPane::updateFilter()
{
    if (RunControlTab * const tab = currentTab()) {
        tab->window->updateFilterProperties(filterText(), filterCaseSensitivity(),
                                            filterUsesRegexp(), filterIsInverted());
    }
}

const QList<Core::OutputWindow *> AppOutputPane::outputWindows() const
{
    QList<Core::OutputWindow *> windows;
    for (const RunControlTab &tab : std::as_const(m_runControlTabs)) {
        if (tab.window)
            windows << tab.window;
    }
    return windows;
}

void AppOutputPane::ensureWindowVisible(Core::OutputWindow *ow)
{
    m_tabWidget->setCurrentWidget(ow);
}

void AppOutputPane::createNewOutputWindow(RunControl *rc)
{
    QTC_ASSERT(rc, return);

    auto runControlChanged = [this, rc] {
        RunControl *current = currentRunControl();
        if (current && current == rc)
            enableButtons(current); // RunControl::isRunning() cannot be trusted in signal handler.
    };

    connect(rc, &RunControl::aboutToStart, this, runControlChanged);
    connect(rc, &RunControl::started, this, runControlChanged);
    connect(rc, &RunControl::stopped, this, [this, rc] {
        QTimer::singleShot(0, this, [this, rc] { runControlFinished(rc); });
        for (const RunControlTab &t : std::as_const(m_runControlTabs)) {
            if (t.runControl == rc) {
                if (t.window)
                    t.window->flush();
                break;
            }
        }
    });
    connect(rc, &RunControl::applicationProcessHandleChanged,
            this, &AppOutputPane::enableDefaultButtons);
    connect(rc, &RunControl::appendMessage,
            this, [this, rc](const QString &out, OutputFormat format) {
                appendMessage(rc, out, format);
            });

    // First look if we can reuse a tab
    const CommandLine thisCommand = rc->commandLine();
    const FilePath thisWorkingDirectory = rc->workingDirectory();
    const Environment thisEnvironment = rc->environment();
    const auto tab = std::find_if(m_runControlTabs.begin(), m_runControlTabs.end(),
                                  [&](const RunControlTab &tab) {
        if (!tab.runControl || tab.runControl->isRunning() || tab.runControl->isStarting())
            return false;
        return thisCommand == tab.runControl->commandLine()
                && thisWorkingDirectory == tab.runControl->workingDirectory()
                && thisEnvironment == tab.runControl->environment();
    });
    if (tab != m_runControlTabs.end()) {
        // Reuse this tab
        if (tab->runControl)
            delete tab->runControl;

        tab->runControl = rc;
        tab->window->reset();
        rc->setupFormatter(tab->window->outputFormatter());

        handleOldOutput(tab->window);

        // Update the title.
        const int tabIndex = m_tabWidget->indexOf(tab->window);
        QTC_ASSERT(tabIndex != -1, return);
        m_tabWidget->setTabText(tabIndex, rc->displayName());

        tab->window->scrollToBottom();
        qCDebug(appOutputLog) << "AppOutputPane::createNewOutputWindow: Reusing tab"
                              << tabIndex << "for" << rc;
        return;
    }
    // Create new
    static int counter = 0;
    Id contextId = Id(C_APP_OUTPUT).withSuffix(counter++);
    Core::Context context(contextId);
    Core::OutputWindow *ow = new Core::OutputWindow(context, SETTINGS_KEY, m_tabWidget);
    ow->setWindowTitle(Tr::tr("Application Output Window"));
    ow->setWindowIcon(Icons::WINDOW.icon());
    ow->setWordWrapEnabled(m_settings.wrapOutput);
    ow->setMaxCharCount(m_settings.maxCharCount);

    auto updateFontSettings = [ow] {
        ow->setBaseFont(TextEditor::TextEditorSettings::fontSettings().font());
    };

    auto updateBehaviorSettings = [ow] {
        ow->setWheelZoomEnabled(
                    TextEditor::TextEditorSettings::behaviorSettings().m_scrollWheelZooming);
    };

    updateFontSettings();
    updateBehaviorSettings();

    connect(ow, &Core::OutputWindow::wheelZoom, this, [this, ow]() {
        float fontZoom = ow->fontZoom();
        for (const RunControlTab &tab : std::as_const(m_runControlTabs))
            tab.window->setFontZoom(fontZoom);
    });
    connect(TextEditor::TextEditorSettings::instance(), &TextEditor::TextEditorSettings::fontSettingsChanged,
            ow, updateFontSettings);
    connect(TextEditor::TextEditorSettings::instance(), &TextEditor::TextEditorSettings::behaviorSettingsChanged,
            ow, updateBehaviorSettings);

    m_runControlTabs.push_back(RunControlTab(rc, ow));
    m_tabWidget->addTab(ow, rc->displayName());
    qCDebug(appOutputLog) << "AppOutputPane::createNewOutputWindow: Adding tab for" << rc;
    updateCloseActions();
    setFilteringEnabled(m_tabWidget->count() > 0);
}

void AppOutputPane::handleOldOutput(Core::OutputWindow *window) const
{
    if (m_settings.cleanOldOutput)
        window->clear();
    else
        window->grayOutOldContent();
}

void AppOutputPane::updateFromSettings()
{
    for (const RunControlTab &tab : std::as_const(m_runControlTabs)) {
        tab.window->setWordWrapEnabled(m_settings.wrapOutput);
        tab.window->setMaxCharCount(m_settings.maxCharCount);
    }
}

void AppOutputPane::appendMessage(RunControl *rc, const QString &out, OutputFormat format)
{
    RunControlTab * const tab = tabFor(rc);
    if (!tab)
        return;

    QString stringToWrite;
    if (format == NormalMessageFormat || format == ErrorMessageFormat) {
        stringToWrite = QTime::currentTime().toString();
        stringToWrite += ": ";
    }
    stringToWrite += out;
    tab->window->appendMessage(stringToWrite, format);

    if (format != NormalMessageFormat) {
        switch (tab->behaviorOnOutput) {
        case AppOutputPaneMode::FlashOnOutput:
            flash();
            break;
        case AppOutputPaneMode::PopupOnFirstOutput:
            tab->behaviorOnOutput = AppOutputPaneMode::FlashOnOutput;
            Q_FALLTHROUGH();
        case AppOutputPaneMode::PopupOnOutput:
            popup(NoModeSwitch);
            break;
        }
    }
}

void AppOutputPane::setSettings(const AppOutputSettings &settings)
{
    m_settings = settings;
    storeSettings();
    updateFromSettings();
}

const AppOutputPaneMode kRunOutputModeDefault = AppOutputPaneMode::PopupOnFirstOutput;
const AppOutputPaneMode kDebugOutputModeDefault = AppOutputPaneMode::FlashOnOutput;
const bool kCleanOldOutputDefault = false;
const bool kMergeChannelsDefault = false;
const bool kWrapOutputDefault = true;

void AppOutputPane::storeSettings() const
{
    QtcSettings *const s = Core::ICore::settings();
    s->setValueWithDefault(POP_UP_FOR_RUN_OUTPUT_KEY,
                           int(m_settings.runOutputMode),
                           int(kRunOutputModeDefault));
    s->setValueWithDefault(POP_UP_FOR_DEBUG_OUTPUT_KEY,
                           int(m_settings.debugOutputMode),
                           int(kDebugOutputModeDefault));
    s->setValueWithDefault(CLEAN_OLD_OUTPUT_KEY, m_settings.cleanOldOutput, kCleanOldOutputDefault);
    s->setValueWithDefault(MERGE_CHANNELS_KEY, m_settings.mergeChannels, kMergeChannelsDefault);
    s->setValueWithDefault(WRAP_OUTPUT_KEY, m_settings.wrapOutput, kWrapOutputDefault);
    s->setValueWithDefault(MAX_LINES_KEY,
                           m_settings.maxCharCount / 100,
                           Core::Constants::DEFAULT_MAX_CHAR_COUNT / 100);
}

void AppOutputPane::loadSettings()
{
    QtcSettings * const s = Core::ICore::settings();
    const auto modeFromSettings = [s](const Key key, AppOutputPaneMode defaultValue) {
        return static_cast<AppOutputPaneMode>(s->value(key, int(defaultValue)).toInt());
    };
    m_settings.runOutputMode = modeFromSettings(POP_UP_FOR_RUN_OUTPUT_KEY, kRunOutputModeDefault);
    m_settings.debugOutputMode = modeFromSettings(POP_UP_FOR_DEBUG_OUTPUT_KEY,
                                                  kDebugOutputModeDefault);
    m_settings.cleanOldOutput = s->value(CLEAN_OLD_OUTPUT_KEY, kCleanOldOutputDefault).toBool();
    m_settings.mergeChannels = s->value(MERGE_CHANNELS_KEY, kMergeChannelsDefault).toBool();
    m_settings.wrapOutput = s->value(WRAP_OUTPUT_KEY, kWrapOutputDefault).toBool();
    m_settings.maxCharCount = s->value(MAX_LINES_KEY,
                                       Core::Constants::DEFAULT_MAX_CHAR_COUNT / 100).toInt() * 100;
}

void AppOutputPane::showTabFor(RunControl *rc)
{
    if (RunControlTab * const tab = tabFor(rc))
        m_tabWidget->setCurrentWidget(tab->window);
}

void AppOutputPane::setBehaviorOnOutput(RunControl *rc, AppOutputPaneMode mode)
{
    if (RunControlTab * const tab = tabFor(rc))
        tab->behaviorOnOutput = mode;
}

void AppOutputPane::reRunRunControl()
{
    RunControlTab * const tab = currentTab();
    QTC_ASSERT(tab, return);
    QTC_ASSERT(tab->runControl, return);
    QTC_ASSERT(!tab->runControl->isRunning(), return);

    handleOldOutput(tab->window);
    tab->window->scrollToBottom();
    tab->runControl->initiateReStart();
}

void AppOutputPane::attachToRunControl()
{
    RunControl * const rc = currentRunControl();
    QTC_ASSERT(rc, return);
    QTC_ASSERT(rc->isRunning(), return);
    ExtensionSystem::Invoker<void>(debuggerPlugin(), "attachExternalApplication", rc);
}

void AppOutputPane::stopRunControl()
{
    RunControl * const rc = currentRunControl();
    QTC_ASSERT(rc, return);

    if (rc->isRunning()) {
        if (optionallyPromptToStop(rc)) {
            rc->initiateStop();
            enableButtons(rc);
        }
    } else {
        QTC_CHECK(false);
        rc->forceStop();
    }

    qCDebug(appOutputLog) << "AppOutputPane::stopRunControl" << rc;
}

void AppOutputPane::closeTabs(CloseTabMode mode)
{
    for (int t = m_tabWidget->count() - 1; t >= 0; t--)
        closeTab(t, mode);
}

QList<RunControl *> AppOutputPane::allRunControls() const
{
    const QList<RunControl *> list = Utils::transform<QList>(m_runControlTabs,[](const RunControlTab &tab) {
        return tab.runControl.data();
    });
    return Utils::filtered(list, [](RunControl *rc) { return rc; });
}

void AppOutputPane::closeTab(int tabIndex, CloseTabMode closeTabMode)
{
    QWidget * const tabWidget = m_tabWidget->widget(tabIndex);
    RunControlTab *tab = tabFor(tabWidget);
    QTC_ASSERT(tab, return);

    RunControl *runControl = tab->runControl;
    Core::OutputWindow *window = tab->window;
    qCDebug(appOutputLog) << "AppOutputPane::closeTab tab" << tabIndex << runControl << window;
    // Prompt user to stop
    if (closeTabMode == CloseTabWithPrompt) {
        if (runControl && runControl->isRunning() && !runControl->promptToStop())
            return;
        // The event loop has run, thus the ordering might have changed, a tab might
        // have been closed, so do some strange things...
        tabIndex = m_tabWidget->indexOf(tabWidget);
        tab = tabFor(tabWidget);
        if (tabIndex == -1 || !tab)
            return;
    }

    m_tabWidget->removeTab(tabIndex);
    delete window;

    Utils::erase(m_runControlTabs, [runControl](const RunControlTab &t) {
        return t.runControl == runControl; });
    if (runControl) {
        if (runControl->isRunning()) {
            QMetaObject::invokeMethod(runControl, [runControl] {
                runControl->setAutoDeleteOnStop(true);
                runControl->initiateStop();
            }, Qt::QueuedConnection);
        } else {
            delete runControl;
        }
    }
    updateCloseActions();
    setFilteringEnabled(m_tabWidget->count() > 0);

    if (m_runControlTabs.isEmpty())
        hide();
}

bool AppOutputPane::optionallyPromptToStop(RunControl *runControl)
{
    ProjectExplorerSettings settings = ProjectExplorerPlugin::projectExplorerSettings();
    if (!runControl->promptToStop(&settings.prompToStopRunControl))
        return false;
    ProjectExplorerPlugin::setProjectExplorerSettings(settings);
    return true;
}

void AppOutputPane::projectRemoved()
{
    tabChanged(m_tabWidget->currentIndex());
}

void AppOutputPane::enableDefaultButtons()
{
    enableButtons(currentRunControl());
}

void AppOutputPane::zoomIn(int range)
{
    for (const RunControlTab &tab : std::as_const(m_runControlTabs))
        tab.window->zoomIn(range);
}

void AppOutputPane::zoomOut(int range)
{
    for (const RunControlTab &tab : std::as_const(m_runControlTabs))
        tab.window->zoomOut(range);
}

void AppOutputPane::resetZoom()
{
    for (const RunControlTab &tab : std::as_const(m_runControlTabs))
        tab.window->resetZoom();
}

void AppOutputPane::enableButtons(const RunControl *rc)
{
    if (rc) {
        const bool isRunning = rc->isRunning();
        m_reRunButton->setEnabled(rc->isStopped() && rc->supportsReRunning());
        m_reRunButton->setIcon(rc->icon().icon());
        m_stopAction->setEnabled(isRunning);
        if (isRunning && debuggerPlugin() && rc->applicationProcessHandle().isValid()) {
            m_attachButton->setEnabled(true);
            ProcessHandle h = rc->applicationProcessHandle();
            QString tip = h.isValid() ? Tr::tr("PID %1").arg(h.pid())
                                      : Tr::tr("Invalid");
            m_attachButton->setToolTip(msgAttachDebuggerTooltip(tip));
        } else {
            m_attachButton->setEnabled(false);
            m_attachButton->setToolTip(msgAttachDebuggerTooltip());
        }
        setZoomButtonsEnabled(true);
    } else {
        m_reRunButton->setEnabled(false);
        m_reRunButton->setIcon(Utils::Icons::RUN_SMALL_TOOLBAR.icon());
        m_attachButton->setEnabled(false);
        m_attachButton->setToolTip(msgAttachDebuggerTooltip());
        m_stopAction->setEnabled(false);
        setZoomButtonsEnabled(false);
    }
    m_formatterWidget->setVisible(m_formatterWidget->layout()->count());
}

void AppOutputPane::tabChanged(int i)
{
    RunControlTab * const controlTab = tabFor(m_tabWidget->widget(i));
    if (i != -1 && controlTab) {
        controlTab->window->updateFilterProperties(filterText(), filterCaseSensitivity(),
                                                   filterUsesRegexp(), filterIsInverted());
        enableButtons(controlTab->runControl);
    } else {
        enableDefaultButtons();
    }
}

void AppOutputPane::contextMenuRequested(const QPoint &pos, int index)
{
    const QList<QAction *> actions = {m_closeCurrentTabAction, m_closeAllTabsAction, m_closeOtherTabsAction};
    QAction *action = QMenu::exec(actions, m_tabWidget->mapToGlobal(pos), nullptr, m_tabWidget);
    if (action == m_closeAllTabsAction) {
        closeTabs(AppOutputPane::CloseTabWithPrompt);
        return;
    }

    const int currentIdx = index != -1 ? index : m_tabWidget->currentIndex();
    if (action == m_closeCurrentTabAction) {
        if (currentIdx >= 0)
            closeTab(currentIdx);
    } else if (action == m_closeOtherTabsAction) {
        for (int t = m_tabWidget->count() - 1; t >= 0; t--)
            if (t != currentIdx)
                closeTab(t);
    }
}

void AppOutputPane::runControlFinished(RunControl *runControl)
{
    const RunControlTab * const tab = tabFor(runControl);

    // This slot is queued, so the stop() call in closeTab might lead to this slot, after closeTab already cleaned up
    if (!tab)
        return;

    // Enable buttons for current
    RunControl *current = currentRunControl();

    qCDebug(appOutputLog) << "AppOutputPane::runControlFinished" << runControl
                          << m_tabWidget->indexOf(tab->window)
                          << "current" << current << m_runControlTabs.size();

    if (current && current == runControl)
        enableButtons(current);

    ProjectExplorerPlugin::updateRunActions();

    const bool isRunning = Utils::anyOf(m_runControlTabs, [](const RunControlTab &rt) {
        return rt.runControl && rt.runControl->isRunning();
    });

    if (!isRunning)
        WinDebugInterface::stop();
}

bool AppOutputPane::canNext() const
{
    return false;
}

bool AppOutputPane::canPrevious() const
{
    return false;
}

void AppOutputPane::goToNext()
{

}

void AppOutputPane::goToPrev()
{

}

bool AppOutputPane::canNavigate() const
{
    return false;
}

class AppOutputSettingsWidget : public Core::IOptionsPageWidget
{
public:
    AppOutputSettingsWidget()
    {
        const AppOutputSettings &settings = ProjectExplorerPlugin::appOutputSettings();
        m_wrapOutputCheckBox.setText(Tr::tr("Word-wrap output"));
        m_wrapOutputCheckBox.setChecked(settings.wrapOutput);
        m_cleanOldOutputCheckBox.setText(Tr::tr("Clear old output on a new run"));
        m_cleanOldOutputCheckBox.setChecked(settings.cleanOldOutput);
        m_mergeChannelsCheckBox.setText(Tr::tr("Merge stderr and stdout"));
        m_mergeChannelsCheckBox.setChecked(settings.mergeChannels);
        for (QComboBox * const modeComboBox
             : {&m_runOutputModeComboBox, &m_debugOutputModeComboBox}) {
            modeComboBox->addItem(Tr::tr("Always"), int(AppOutputPaneMode::PopupOnOutput));
            modeComboBox->addItem(Tr::tr("Never"), int(AppOutputPaneMode::FlashOnOutput));
            modeComboBox->addItem(Tr::tr("On First Output Only"),
                                  int(AppOutputPaneMode::PopupOnFirstOutput));
        }
        m_runOutputModeComboBox.setCurrentIndex(m_runOutputModeComboBox
                                                .findData(int(settings.runOutputMode)));
        m_debugOutputModeComboBox.setCurrentIndex(m_debugOutputModeComboBox
                                                  .findData(int(settings.debugOutputMode)));
        m_maxCharsBox.setMaximum(100000000);
        m_maxCharsBox.setValue(settings.maxCharCount);
        const auto layout = new QVBoxLayout(this);
        layout->addWidget(&m_wrapOutputCheckBox);
        layout->addWidget(&m_cleanOldOutputCheckBox);
        layout->addWidget(&m_mergeChannelsCheckBox);
        const auto maxCharsLayout = new QHBoxLayout;
        const QString msg = Tr::tr("Limit output to %1 characters");
        const QStringList parts = msg.split("%1") << QString() << QString();
        maxCharsLayout->addWidget(new QLabel(parts.at(0).trimmed()));
        maxCharsLayout->addWidget(&m_maxCharsBox);
        maxCharsLayout->addWidget(new QLabel(parts.at(1).trimmed()));
        maxCharsLayout->addStretch(1);
        const auto outputModeLayout = new QFormLayout;
        outputModeLayout->addRow(Tr::tr("Open Application Output when running:"), &m_runOutputModeComboBox);
        outputModeLayout->addRow(Tr::tr("Open Application Output when debugging:"),
                                 &m_debugOutputModeComboBox);
        layout->addLayout(outputModeLayout);
        layout->addLayout(maxCharsLayout);
        layout->addStretch(1);
    }

    void apply() final
    {
        AppOutputSettings s;
        s.wrapOutput = m_wrapOutputCheckBox.isChecked();
        s.cleanOldOutput = m_cleanOldOutputCheckBox.isChecked();
        s.mergeChannels = m_mergeChannelsCheckBox.isChecked();
        s.runOutputMode = static_cast<AppOutputPaneMode>(
                    m_runOutputModeComboBox.currentData().toInt());
        s.debugOutputMode = static_cast<AppOutputPaneMode>(
                    m_debugOutputModeComboBox.currentData().toInt());
        s.maxCharCount = m_maxCharsBox.value();

        ProjectExplorerPlugin::setAppOutputSettings(s);
    }

private:
    QCheckBox m_wrapOutputCheckBox;
    QCheckBox m_cleanOldOutputCheckBox;
    QCheckBox m_mergeChannelsCheckBox;
    QComboBox m_runOutputModeComboBox;
    QComboBox m_debugOutputModeComboBox;
    QSpinBox m_maxCharsBox;
};

AppOutputSettingsPage::AppOutputSettingsPage()
{
    setId(OPTIONS_PAGE_ID);
    setDisplayName(Tr::tr("Application Output"));
    setCategory(Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new AppOutputSettingsWidget; });
}

} // namespace Internal
} // namespace ProjectExplorer

#include "appoutputpane.moc"

