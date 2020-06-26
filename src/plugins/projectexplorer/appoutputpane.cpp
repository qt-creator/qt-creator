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

#include "appoutputpane.h"

#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorericons.h"
#include "runcontrol.h"
#include "session.h"
#include "windebuginterface.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/find/basetextfind.h>
#include <coreplugin/icore.h>
#include <coreplugin/outputwindow.h>
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

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

const char OPTIONS_PAGE_ID[] = "B.ProjectExplorer.AppOutputOptions";


static QObject *debuggerPlugin()
{
    return ExtensionSystem::PluginManager::getObjectByName("DebuggerPlugin");
}

static QString msgAttachDebuggerTooltip(const QString &handleDescription = QString())
{
    return handleDescription.isEmpty() ?
           AppOutputPane::tr("Attach debugger to this process") :
           AppOutputPane::tr("Attach debugger to %1").arg(handleDescription);
}

namespace {
const char SETTINGS_KEY[] = "ProjectExplorer/AppOutput/Zoom";
const char C_APP_OUTPUT[] = "ProjectExplorer.ApplicationOutput";
const char POP_UP_FOR_RUN_OUTPUT_KEY[] = "ProjectExplorer/Settings/ShowRunOutput";
const char POP_UP_FOR_DEBUG_OUTPUT_KEY[] = "ProjectExplorer/Settings/ShowDebugOutput";
const char CLEAN_OLD_OUTPUT_KEY[] = "ProjectExplorer/Settings/CleanOldAppOutput";
const char MERGE_CHANNELS_KEY[] = "ProjectExplorer/Settings/MergeStdErrAndStdOut";
const char WRAP_OUTPUT_KEY[] = "ProjectExplorer/Settings/WrapAppOutput";
const char MAX_LINES_KEY[] = "ProjectExplorer/Settings/MaxAppOutputLines";
}

namespace ProjectExplorer {
namespace Internal {

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
    if (runControl && w)
        w->setLineParsers(runControl->createOutputParsers());
}

AppOutputPane::AppOutputPane() :
    m_mainWidget(new QWidget),
    m_tabWidget(new TabWidget),
    m_stopAction(new QAction(tr("Stop"), this)),
    m_closeCurrentTabAction(new QAction(tr("Close Tab"), this)),
    m_closeAllTabsAction(new QAction(tr("Close All Tabs"), this)),
    m_closeOtherTabsAction(new QAction(tr("Close Other Tabs"), this)),
    m_reRunButton(new QToolButton),
    m_stopButton(new QToolButton),
    m_attachButton(new QToolButton),
    m_settingsButton(new QToolButton),
    m_formatterWidget(new QWidget)
{
    setObjectName("AppOutputPane"); // Used in valgrind engine
    loadSettings();

    // Rerun
    m_reRunButton->setIcon(Utils::Icons::RUN_SMALL_TOOLBAR.icon());
    m_reRunButton->setToolTip(tr("Re-run this run-configuration."));
    m_reRunButton->setEnabled(false);
    connect(m_reRunButton, &QToolButton::clicked,
            this, &AppOutputPane::reRunRunControl);

    // Stop
    m_stopAction->setIcon(Utils::Icons::STOP_SMALL_TOOLBAR.icon());
    m_stopAction->setToolTip(tr("Stop running program."));
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

    connect(this, &Core::IOutputPane::zoomIn, this, &AppOutputPane::zoomIn);
    connect(this, &Core::IOutputPane::zoomOut, this, &AppOutputPane::zoomOut);
    connect(this, &IOutputPane::resetZoom, this, &AppOutputPane::resetZoom);

    m_settingsButton->setToolTip(tr("Open Settings Page"));
    m_settingsButton->setIcon(Utils::Icons::SETTINGS_TOOLBAR.icon());
    connect(m_settingsButton, &QToolButton::clicked, this, [] {
        Core::ICore::showOptionsDialog(OPTIONS_PAGE_ID);
    });

    auto formatterWidgetsLayout = new QHBoxLayout;
    formatterWidgetsLayout->setContentsMargins(QMargins());
    m_formatterWidget->setLayout(formatterWidgetsLayout);

    // Spacer (?)

    auto *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, [this](int index) { closeTab(index); });
    layout->addWidget(m_tabWidget);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &AppOutputPane::tabChanged);
    connect(m_tabWidget, &TabWidget::contextMenuRequested,
            this, &AppOutputPane::contextMenuRequested);

    m_mainWidget->setLayout(layout);

    connect(SessionManager::instance(), &SessionManager::aboutToUnloadSession,
            this, &AppOutputPane::aboutToUnloadSession);

    setupFilterUi("AppOutputPane.Filter");
    setFilteringEnabled(false);
    setZoomButtonsEnabled(false);
    setupContext("Core.AppOutputPane", m_mainWidget);
}

AppOutputPane::~AppOutputPane()
{
    qCDebug(appOutputLog) << "AppOutputPane::~AppOutputPane: Entries left" << m_runControlTabs.size();

    for (const RunControlTab &rt : qAsConst(m_runControlTabs)) {
        delete rt.window;
        delete rt.runControl;
    }
    delete m_mainWidget;
}

int AppOutputPane::currentIndex() const
{
    if (const QWidget *w = m_tabWidget->currentWidget())
        return indexOf(w);
    return -1;
}

RunControl *AppOutputPane::currentRunControl() const
{
    const int index = currentIndex();
    if (index != -1)
        return m_runControlTabs.at(index).runControl;
    return nullptr;
}

int AppOutputPane::indexOf(const RunControl *rc) const
{
    for (int i = m_runControlTabs.size() - 1; i >= 0; i--)
        if (m_runControlTabs.at(i).runControl == rc)
            return i;
    return -1;
}

int AppOutputPane::indexOf(const QWidget *outputWindow) const
{
    for (int i = m_runControlTabs.size() - 1; i >= 0; i--)
        if (m_runControlTabs.at(i).window == outputWindow)
            return i;
    return -1;
}

int AppOutputPane::tabWidgetIndexOf(int runControlIndex) const
{
    if (runControlIndex >= 0 && runControlIndex < m_runControlTabs.size())
        return m_tabWidget->indexOf(m_runControlTabs.at(runControlIndex).window);
    return -1;
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
    return m_mainWidget;
}

QList<QWidget*> AppOutputPane::toolBarWidgets() const
{
    return QList<QWidget *>{m_reRunButton, m_stopButton, m_attachButton, m_settingsButton,
                m_formatterWidget} + IOutputPane::toolBarWidgets();
}

QString AppOutputPane::displayName() const
{
    return tr("Application Output");
}

int AppOutputPane::priorityInStatusBar() const
{
    return 60;
}

void AppOutputPane::clearContents()
{
    auto *currentWindow = qobject_cast<Core::OutputWindow *>(m_tabWidget->currentWidget());
    if (currentWindow)
        currentWindow->clear();
}

void AppOutputPane::visibilityChanged(bool /* b */)
{
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
    const int index = currentIndex();
    if (index != -1) {
        m_runControlTabs.at(index).window->updateFilterProperties(
                    filterText(), filterCaseSensitivity(), filterUsesRegexp(), filterIsInverted());
    }
}

void AppOutputPane::createNewOutputWindow(RunControl *rc)
{
    QTC_ASSERT(rc, return);

    connect(rc, &RunControl::aboutToStart,
            this, &AppOutputPane::slotRunControlChanged);
    connect(rc, &RunControl::started,
            this, &AppOutputPane::slotRunControlChanged);
    connect(rc, &RunControl::stopped,
            this, &AppOutputPane::slotRunControlFinished);
    connect(rc, &RunControl::applicationProcessHandleChanged,
            this, &AppOutputPane::enableDefaultButtons);
    connect(rc, &RunControl::appendMessage,
            this, [this, rc](const QString &out, Utils::OutputFormat format) {
                appendMessage(rc, out, format);
            });

    // First look if we can reuse a tab
    const Runnable thisRunnable = rc->runnable();
    const int tabIndex = Utils::indexOf(m_runControlTabs, [&](const RunControlTab &tab) {
        if (!tab.runControl || tab.runControl->isRunning())
            return false;
        const Runnable otherRunnable = tab.runControl->runnable();
        return thisRunnable.executable == otherRunnable.executable
                && thisRunnable.commandLineArguments == otherRunnable.commandLineArguments
                && thisRunnable.workingDirectory == otherRunnable.workingDirectory
                && thisRunnable.environment == otherRunnable.environment;
    });
    if (tabIndex != -1) {
        RunControlTab &tab = m_runControlTabs[tabIndex];
        // Reuse this tab
        if (tab.runControl)
            tab.runControl->initiateFinish();
        tab.runControl = rc;
        tab.window->setLineParsers(rc->createOutputParsers());

        handleOldOutput(tab.window);

        // Update the title.
        m_tabWidget->setTabText(tabIndex, rc->displayName());

        tab.window->scrollToBottom();
        qCDebug(appOutputLog) << "AppOutputPane::createNewOutputWindow: Reusing tab"
                              << tabIndex << "for" << rc;
        return;
    }
    // Create new
    static int counter = 0;
    Utils::Id contextId = Utils::Id(C_APP_OUTPUT).withSuffix(counter++);
    Core::Context context(contextId);
    Core::OutputWindow *ow = new Core::OutputWindow(context, SETTINGS_KEY, m_tabWidget);
    ow->setWindowTitle(tr("Application Output Window"));
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
        for (const RunControlTab &tab : qAsConst(m_runControlTabs))
            tab.window->setFontZoom(fontZoom);
    });
    connect(TextEditor::TextEditorSettings::instance(), &TextEditor::TextEditorSettings::fontSettingsChanged,
            ow, updateFontSettings);
    connect(TextEditor::TextEditorSettings::instance(), &TextEditor::TextEditorSettings::behaviorSettingsChanged,
            ow, updateBehaviorSettings);

    auto *agg = new Aggregation::Aggregate;
    agg->add(ow);
    agg->add(new Core::BaseTextFind(ow));
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
    for (const RunControlTab &tab : qAsConst(m_runControlTabs)) {
        tab.window->setWordWrapEnabled(m_settings.wrapOutput);
        tab.window->setMaxCharCount(m_settings.maxCharCount);
    }
}

void AppOutputPane::appendMessage(RunControl *rc, const QString &out, Utils::OutputFormat format)
{
    const int index = indexOf(rc);
    if (index != -1) {
        Core::OutputWindow *window = m_runControlTabs.at(index).window;
        QString stringToWrite;
        if (format == Utils::NormalMessageFormat || format == Utils::ErrorMessageFormat) {
            stringToWrite = QTime::currentTime().toString();
            stringToWrite += ": ";
        }
        stringToWrite += out;
        window->appendMessage(stringToWrite, format);
        if (format != Utils::NormalMessageFormat) {
            RunControlTab &tab = m_runControlTabs[index];
            switch (tab.behaviorOnOutput) {
            case AppOutputPaneMode::FlashOnOutput:
                flash();
                break;
            case AppOutputPaneMode::PopupOnFirstOutput:
                tab.behaviorOnOutput = AppOutputPaneMode::FlashOnOutput;
                Q_FALLTHROUGH();
            case AppOutputPaneMode::PopupOnOutput:
                popup(NoModeSwitch);
                break;
            }
        }
    }
}

void AppOutputPane::setSettings(const AppOutputSettings &settings)
{
    m_settings = settings;
    storeSettings();
    updateFromSettings();
}

void AppOutputPane::storeSettings() const
{
    QSettings * const s = Core::ICore::settings();
    s->setValue(POP_UP_FOR_RUN_OUTPUT_KEY, int(m_settings.runOutputMode));
    s->setValue(POP_UP_FOR_DEBUG_OUTPUT_KEY, int(m_settings.debugOutputMode));
    s->setValue(CLEAN_OLD_OUTPUT_KEY, m_settings.cleanOldOutput);
    s->setValue(MERGE_CHANNELS_KEY, m_settings.mergeChannels);
    s->setValue(WRAP_OUTPUT_KEY, m_settings.wrapOutput);
    s->setValue(MAX_LINES_KEY, m_settings.maxCharCount / 100);
}

void AppOutputPane::loadSettings()
{
    QSettings * const s = Core::ICore::settings();
    const auto modeFromSettings = [s](const QString key, AppOutputPaneMode defaultValue) {
        return static_cast<AppOutputPaneMode>(s->value(key, int(defaultValue)).toInt());
    };
    m_settings.runOutputMode = modeFromSettings(POP_UP_FOR_RUN_OUTPUT_KEY,
                                                AppOutputPaneMode::PopupOnFirstOutput);
    m_settings.debugOutputMode = modeFromSettings(POP_UP_FOR_DEBUG_OUTPUT_KEY,
                                                  AppOutputPaneMode::FlashOnOutput);
    m_settings.cleanOldOutput = s->value(CLEAN_OLD_OUTPUT_KEY, false).toBool();
    m_settings.mergeChannels = s->value(MERGE_CHANNELS_KEY, false).toBool();
    m_settings.wrapOutput = s->value(WRAP_OUTPUT_KEY, true).toBool();
    m_settings.maxCharCount = s->value(MAX_LINES_KEY,
                                       Core::Constants::DEFAULT_MAX_CHAR_COUNT).toInt() * 100;
}

void AppOutputPane::showTabFor(RunControl *rc)
{
    m_tabWidget->setCurrentIndex(tabWidgetIndexOf(indexOf(rc)));
}

void AppOutputPane::setBehaviorOnOutput(RunControl *rc, AppOutputPaneMode mode)
{
    const int index = indexOf(rc);
    if (index != -1)
        m_runControlTabs[index].behaviorOnOutput = mode;
}

void AppOutputPane::reRunRunControl()
{
    const int index = currentIndex();
    const RunControlTab &tab = m_runControlTabs.at(index);
    QTC_ASSERT(tab.runControl, return);
    QTC_ASSERT(index != -1 && !tab.runControl->isRunning(), return);

    handleOldOutput(tab.window);
    tab.window->scrollToBottom();
    tab.runControl->initiateReStart();
}

void AppOutputPane::attachToRunControl()
{
    const int index = currentIndex();
    QTC_ASSERT(index != -1, return);
    RunControl *rc = m_runControlTabs.at(index).runControl;
    QTC_ASSERT(rc && rc->isRunning(), return);
    ExtensionSystem::Invoker<void>(debuggerPlugin(), "attachExternalApplication", rc);
}

void AppOutputPane::stopRunControl()
{
    const int index = currentIndex();
    QTC_ASSERT(index != -1, return);
    RunControl *rc = m_runControlTabs.at(index).runControl;
    QTC_ASSERT(rc, return);

    if (rc->isRunning() && optionallyPromptToStop(rc))
        rc->initiateStop();
    else {
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
    int index = indexOf(m_tabWidget->widget(tabIndex));
    QTC_ASSERT(index != -1, return);

    RunControl *runControl = m_runControlTabs[index].runControl;
    Core::OutputWindow *window = m_runControlTabs[index].window;
    qCDebug(appOutputLog) << "AppOutputPane::closeTab tab" << tabIndex << runControl << window;
    // Prompt user to stop
    if (closeTabMode == CloseTabWithPrompt) {
        QWidget *tabWidget = m_tabWidget->widget(tabIndex);
        if (runControl && runControl->isRunning() && !runControl->promptToStop())
            return;
        // The event loop has run, thus the ordering might have changed, a tab might
        // have been closed, so do some strange things...
        tabIndex = m_tabWidget->indexOf(tabWidget);
        index = indexOf(tabWidget);
        if (tabIndex == -1 || index == -1)
            return;
    }

    m_tabWidget->removeTab(tabIndex);
    delete window;

    if (runControl)
        runControl->initiateFinish(); // Will self-destruct.
    m_runControlTabs.removeAt(index);
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
    for (const RunControlTab &tab : qAsConst(m_runControlTabs))
        tab.window->zoomIn(range);
}

void AppOutputPane::zoomOut(int range)
{
    for (const RunControlTab &tab : qAsConst(m_runControlTabs))
        tab.window->zoomOut(range);
}

void AppOutputPane::resetZoom()
{
    for (const RunControlTab &tab : qAsConst(m_runControlTabs))
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
            Utils::ProcessHandle h = rc->applicationProcessHandle();
            QString tip = h.isValid() ? RunControl::tr("PID %1").arg(h.pid())
                                      : RunControl::tr("Invalid");
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
    const int index = indexOf(m_tabWidget->widget(i));
    if (i != -1 && index != -1) {
        const RunControlTab &controlTab = m_runControlTabs[index];
        controlTab.window->updateFilterProperties(filterText(), filterCaseSensitivity(),
                                                  filterUsesRegexp(), filterIsInverted());
        enableButtons(controlTab.runControl);
    } else {
        enableDefaultButtons();
    }
}

void AppOutputPane::contextMenuRequested(const QPoint &pos, int index)
{
    const QList<QAction *> actions = {m_closeCurrentTabAction, m_closeAllTabsAction, m_closeOtherTabsAction};
    QAction *action = QMenu::exec(actions, m_tabWidget->mapToGlobal(pos), nullptr, m_tabWidget);
    const int currentIdx = index != -1 ? index : currentIndex();
    if (action == m_closeCurrentTabAction) {
        if (currentIdx >= 0)
            closeTab(currentIdx);
    } else if (action == m_closeAllTabsAction) {
        closeTabs(AppOutputPane::CloseTabWithPrompt);
    } else if (action == m_closeOtherTabsAction) {
        for (int t = m_tabWidget->count() - 1; t >= 0; t--)
            if (t != currentIdx)
                closeTab(t);
    }
}

void AppOutputPane::slotRunControlChanged()
{
    RunControl *current = currentRunControl();
    if (current && current == sender())
        enableButtons(current); // RunControl::isRunning() cannot be trusted in signal handler.
}

void AppOutputPane::slotRunControlFinished()
{
    auto *rc = qobject_cast<RunControl *>(sender());
    QTimer::singleShot(0, this, [this, rc]() { slotRunControlFinished2(rc); });
    for (const RunControlTab &t : m_runControlTabs) {
        if (t.runControl == rc) {
            t.window->flush();
            break;
        }
    }
}

void AppOutputPane::slotRunControlFinished2(RunControl *sender)
{
    const int senderIndex = indexOf(sender);

    // This slot is queued, so the stop() call in closeTab might lead to this slot, after closeTab already cleaned up
    if (senderIndex == -1)
        return;

    // Enable buttons for current
    RunControl *current = currentRunControl();

    qCDebug(appOutputLog) << "AppOutputPane::runControlFinished"  << sender << senderIndex
                          << "current" << current << m_runControlTabs.size();

    if (current && current == sender)
        enableButtons(current);

    ProjectExplorerPlugin::updateRunActions();

#ifdef Q_OS_WIN
    const bool isRunning = Utils::anyOf(m_runControlTabs, [](const RunControlTab &rt) {
        return rt.runControl && rt.runControl->isRunning();
    });
    if (!isRunning)
        WinDebugInterface::instance()->stop();
#endif

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
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Internal::AppOutputSettingsPage)
public:
    AppOutputSettingsWidget()
    {
        const AppOutputSettings &settings = ProjectExplorerPlugin::appOutputSettings();
        m_wrapOutputCheckBox.setText(tr("Word-wrap output"));
        m_wrapOutputCheckBox.setChecked(settings.wrapOutput);
        m_cleanOldOutputCheckBox.setText(tr("Clear old output on a new run"));
        m_cleanOldOutputCheckBox.setChecked(settings.cleanOldOutput);
        m_mergeChannelsCheckBox.setText(tr("Merge stderr and stdout"));
        m_mergeChannelsCheckBox.setChecked(settings.mergeChannels);
        for (QComboBox * const modeComboBox
             : {&m_runOutputModeComboBox, &m_debugOutputModeComboBox}) {
            modeComboBox->addItem(tr("Always"), int(AppOutputPaneMode::PopupOnOutput));
            modeComboBox->addItem(tr("Never"), int(AppOutputPaneMode::FlashOnOutput));
            modeComboBox->addItem(tr("On First Output Only"),
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
        const QString msg = tr("Limit output to %1 characters");
        const QStringList parts = msg.split("%1") << QString() << QString();
        maxCharsLayout->addWidget(new QLabel(parts.at(0).trimmed()));
        maxCharsLayout->addWidget(&m_maxCharsBox);
        maxCharsLayout->addWidget(new QLabel(parts.at(1).trimmed()));
        maxCharsLayout->addStretch(1);
        const auto outputModeLayout = new QFormLayout;
        outputModeLayout->addRow(tr("Open pane on output when running:"), &m_runOutputModeComboBox);
        outputModeLayout->addRow(tr("Open pane on output when debugging:"),
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
    setDisplayName(AppOutputSettingsWidget::tr("Application Output"));
    setCategory(Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new AppOutputSettingsWidget; });
}

} // namespace Internal
} // namespace ProjectExplorer

#include "appoutputpane.moc"

