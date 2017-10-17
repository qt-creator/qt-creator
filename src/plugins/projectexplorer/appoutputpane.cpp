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
#include "projectexplorericons.h"
#include "projectexplorersettings.h"
#include "runconfiguration.h"
#include "session.h"
#include "windebuginterface.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/outputwindow.h>
#include <coreplugin/find/basetextfind.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/behaviorsettings.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/invoker.h>

#include <utils/algorithm.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QToolButton>
#include <QTabBar>
#include <QMenu>

#include <QDebug>

enum { debug = 0 };

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

static QObject *debuggerPlugin()
{
    return ExtensionSystem::PluginManager::getObjectByName(QLatin1String("DebuggerPlugin"));
}

static QString msgAttachDebuggerTooltip(const QString &handleDescription = QString())
{
    return handleDescription.isEmpty() ?
           AppOutputPane::tr("Attach debugger to this process") :
           AppOutputPane::tr("Attach debugger to %1").arg(handleDescription);
}

static void replaceAllChildWidgets(QLayout *layout, const QList<QWidget *> &newChildren)
{
    while (QLayoutItem *child = layout->takeAt(0))
        delete child;

    foreach (QWidget *widget, newChildren)
        layout->addWidget(widget);
}

namespace {
const char SETTINGS_KEY[] = "ProjectExplorer/AppOutput/Zoom";
const char C_APP_OUTPUT[] = "ProjectExplorer.ApplicationOutput";
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

}
}

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
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::MiddleButton) {
                m_tabIndexForMiddleClick = tabBar()->tabAt(me->pos());
                event->accept();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
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

AppOutputPane::RunControlTab::RunControlTab(RunControl *rc, Core::OutputWindow *w) :
    runControl(rc), window(w)
{
    if (rc && w)
        w->setFormatter(rc->outputFormatter());
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
    m_zoomInButton(new QToolButton),
    m_zoomOutButton(new QToolButton),
    m_formatterWidget(new QWidget)
{
    setObjectName(QLatin1String("AppOutputPane")); // Used in valgrind engine

    // Rerun
    m_reRunButton->setIcon(Utils::Icons::RUN_SMALL_TOOLBAR.icon());
    m_reRunButton->setToolTip(tr("Re-run this run-configuration"));
    m_reRunButton->setAutoRaise(true);
    m_reRunButton->setEnabled(false);
    connect(m_reRunButton, &QToolButton::clicked,
            this, &AppOutputPane::reRunRunControl);

    // Stop
    m_stopAction->setIcon(Utils::Icons::STOP_SMALL_TOOLBAR.icon());
    m_stopAction->setToolTip(tr("Stop Running Program"));
    m_stopAction->setEnabled(false);

    Core::Command *cmd = Core::ActionManager::registerAction(m_stopAction, Constants::STOP);
    cmd->setDescription(m_stopAction->toolTip());

    m_stopButton->setDefaultAction(cmd->action());
    m_stopButton->setAutoRaise(true);

    connect(m_stopAction, &QAction::triggered,
            this, &AppOutputPane::stopRunControl);

    // Attach
    m_attachButton->setToolTip(msgAttachDebuggerTooltip());
    m_attachButton->setEnabled(false);
    m_attachButton->setIcon(Icons::DEBUG_START_SMALL_TOOLBAR.icon());
    m_attachButton->setAutoRaise(true);

    connect(m_attachButton, &QToolButton::clicked,
            this, &AppOutputPane::attachToRunControl);

    m_zoomInButton->setToolTip(tr("Increase Font Size"));
    m_zoomInButton->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    m_zoomInButton->setAutoRaise(true);

    connect(m_zoomInButton, &QToolButton::clicked,
            this, &AppOutputPane::zoomIn);

    m_zoomOutButton->setToolTip(tr("Decrease Font Size"));
    m_zoomOutButton->setIcon(Utils::Icons::MINUS.icon());
    m_zoomOutButton->setAutoRaise(true);

    connect(m_zoomOutButton, &QToolButton::clicked,
            this, &AppOutputPane::zoomOut);

    auto formatterWidgetsLayout = new QHBoxLayout;
    formatterWidgetsLayout->setContentsMargins(QMargins());
    m_formatterWidget->setLayout(formatterWidgetsLayout);

    // Spacer (?)

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
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

    connect(TextEditor::TextEditorSettings::instance(), &TextEditor::TextEditorSettings::fontSettingsChanged,
            this, &AppOutputPane::updateFontSettings);

    connect(TextEditor::TextEditorSettings::instance(), &TextEditor::TextEditorSettings::behaviorSettingsChanged,
            this, &AppOutputPane::updateBehaviorSettings);

    connect(SessionManager::instance(), &SessionManager::aboutToUnloadSession,
            this, &AppOutputPane::aboutToUnloadSession);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &AppOutputPane::updateFromSettings);

    QSettings *settings = Core::ICore::settings();
    m_zoom = settings->value(QLatin1String(SETTINGS_KEY), 0).toFloat();

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &AppOutputPane::saveSettings);
}

AppOutputPane::~AppOutputPane()
{
    if (debug)
        qDebug() << "OutputPane::~OutputPane: Entries left" << m_runControlTabs.size();

    foreach (const RunControlTab &rt, m_runControlTabs) {
        delete rt.window;
        delete rt.runControl;
    }
    delete m_mainWidget;
}

void AppOutputPane::saveSettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->setValue(QLatin1String(SETTINGS_KEY), m_zoom);
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
        return !rt.runControl->isRunning() || rt.runControl->promptToStop();
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
    return { m_reRunButton, m_stopButton, m_attachButton, m_zoomInButton,
                m_zoomOutButton, m_formatterWidget };
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
    Core::OutputWindow *currentWindow = qobject_cast<Core::OutputWindow *>(m_tabWidget->currentWidget());
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

void AppOutputPane::updateFontSettings()
{
    QFont f = TextEditor::TextEditorSettings::fontSettings().font();
    foreach (const RunControlTab &rcTab, m_runControlTabs)
        rcTab.window->setBaseFont(f);
}

void AppOutputPane::updateBehaviorSettings()
{
    bool zoomEnabled = TextEditor::TextEditorSettings::behaviorSettings().m_scrollWheelZooming;
    foreach (const RunControlTab &rcTab, m_runControlTabs)
        rcTab.window->setWheelZoomEnabled(zoomEnabled);
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
    connect(rc, &RunControl::appendMessageRequested,
            this, &AppOutputPane::appendMessage);

    // First look if we can reuse a tab
    const int tabIndex = Utils::indexOf(m_runControlTabs, [rc](const RunControlTab &tab) {
        return rc->canReUseOutputPane(tab.runControl);
    });
    if (tabIndex != -1) {
        RunControlTab &tab = m_runControlTabs[tabIndex];
        // Reuse this tab
        if (tab.runControl)
            tab.runControl->initiateFinish();
        tab.runControl = rc;
        tab.window->setFormatter(rc->outputFormatter());

        handleOldOutput(tab.window);

        // Update the title.
        m_tabWidget->setTabText(tabIndex, rc->displayName());

        tab.window->scrollToBottom();
        if (debug)
            qDebug() << "OutputPane::createNewOutputWindow: Reusing tab" << tabIndex << " for " << rc;
        return;
    }
    // Create new
    static int counter = 0;
    Core::Id contextId = Core::Id(C_APP_OUTPUT).withSuffix(counter++);
    Core::Context context(contextId);
    Core::OutputWindow *ow = new Core::OutputWindow(context, m_tabWidget);
    ow->setWindowTitle(tr("Application Output Window"));
    ow->setWindowIcon(Icons::WINDOW.icon());
    ow->setWordWrapEnabled(ProjectExplorerPlugin::projectExplorerSettings().wrapAppOutput);
    ow->setMaxLineCount(ProjectExplorerPlugin::projectExplorerSettings().maxAppOutputLines);
    ow->setWheelZoomEnabled(TextEditor::TextEditorSettings::behaviorSettings().m_scrollWheelZooming);
    ow->setBaseFont(TextEditor::TextEditorSettings::fontSettings().font());
    ow->setFontZoom(m_zoom);

    connect(ow, &Core::OutputWindow::wheelZoom, this, [this, ow]() {
        m_zoom = ow->fontZoom();
        foreach (const RunControlTab &tab, m_runControlTabs)
            tab.window->setFontZoom(m_zoom);
    });

    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(ow);
    agg->add(new Core::BaseTextFind(ow));
    m_runControlTabs.push_back(RunControlTab(rc, ow));
    m_tabWidget->addTab(ow, rc->displayName());
    if (debug)
        qDebug() << "OutputPane::createNewOutputWindow: Adding tab for " << rc;
    updateCloseActions();
}

void AppOutputPane::handleOldOutput(Core::OutputWindow *window) const
{
    if (ProjectExplorerPlugin::projectExplorerSettings().cleanOldAppOutput)
        window->clear();
    else
        window->grayOutOldContent();
}

void AppOutputPane::updateFromSettings()
{
    foreach (const RunControlTab &tab, m_runControlTabs) {
        tab.window->setWordWrapEnabled(ProjectExplorerPlugin::projectExplorerSettings().wrapAppOutput);
        tab.window->setMaxLineCount(ProjectExplorerPlugin::projectExplorerSettings().maxAppOutputLines);
    }
}

void AppOutputPane::appendMessage(RunControl *rc, const QString &out, Utils::OutputFormat format)
{
    const int index = indexOf(rc);
    if (index != -1) {
        Core::OutputWindow *window = m_runControlTabs.at(index).window;
        window->appendMessage(out, format);
        if (format != Utils::NormalMessageFormat) {
            if (m_runControlTabs.at(index).behaviorOnOutput == Flash)
                flash();
            else
                popup(NoModeSwitch);
        }
    }
}

void AppOutputPane::showTabFor(RunControl *rc)
{
    m_tabWidget->setCurrentIndex(tabWidgetIndexOf(indexOf(rc)));
}

void AppOutputPane::setBehaviorOnOutput(RunControl *rc, AppOutputPane::BehaviorOnOutput mode)
{
    const int index = indexOf(rc);
    if (index != -1)
        m_runControlTabs[index].behaviorOnOutput = mode;
}

void AppOutputPane::reRunRunControl()
{
    const int index = currentIndex();
    QTC_ASSERT(index != -1 && !m_runControlTabs.at(index).runControl->isRunning(), return);

    RunControlTab &tab = m_runControlTabs[index];

    handleOldOutput(tab.window);
    tab.window->scrollToBottom();
    tab.runControl->initiateReStart();
}

void AppOutputPane::attachToRunControl()
{
    const int index = currentIndex();
    QTC_ASSERT(index != -1, return);
    RunControl *rc = m_runControlTabs.at(index).runControl;
    QTC_ASSERT(rc->isRunning(), return);
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

    if (debug)
        qDebug() << "OutputPane::stopRunControl " << rc;
}

void AppOutputPane::closeTabs(CloseTabMode mode)
{
    for (int t = m_tabWidget->count() - 1; t >= 0; t--)
        closeTab(t, mode);
}

QList<RunControl *> AppOutputPane::allRunControls() const
{
    return Utils::transform<QList>(m_runControlTabs,[](const RunControlTab &tab) {
        return tab.runControl.data();
    });
}

void AppOutputPane::closeTab(int tabIndex, CloseTabMode closeTabMode)
{
    int index = indexOf(m_tabWidget->widget(tabIndex));
    QTC_ASSERT(index != -1, return);

    RunControl *runControl = m_runControlTabs[index].runControl;
    Core::OutputWindow *window = m_runControlTabs[index].window;
    if (debug)
        qDebug() << "OutputPane::closeTab tab " << tabIndex << runControl << window;
    // Prompt user to stop
    if (closeTabMode == CloseTabWithPrompt) {
        QWidget *tabWidget = m_tabWidget->widget(tabIndex);
        if (runControl->isRunning() && !runControl->promptToStop())
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

    runControl->initiateFinish(); // Will self-destruct.
    m_runControlTabs.removeAt(index);
    updateCloseActions();

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

void AppOutputPane::zoomIn()
{
    foreach (const RunControlTab &tab, m_runControlTabs)
        tab.window->zoomIn(1);
    if (m_runControlTabs.isEmpty())
        return;
    m_zoom = m_runControlTabs.first().window->fontZoom();
}

void AppOutputPane::zoomOut()
{
    foreach (const RunControlTab &tab, m_runControlTabs)
        tab.window->zoomOut(1);
    if (m_runControlTabs.isEmpty())
        return;
    m_zoom = m_runControlTabs.first().window->fontZoom();
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
        m_zoomInButton->setEnabled(true);
        m_zoomOutButton->setEnabled(true);

        replaceAllChildWidgets(m_formatterWidget->layout(), rc->outputFormatter() ?
                                   rc->outputFormatter()->toolbarWidgets() :
                                   QList<QWidget *>());
    } else {
        m_reRunButton->setEnabled(false);
        m_reRunButton->setIcon(Utils::Icons::RUN_SMALL_TOOLBAR.icon());
        m_attachButton->setEnabled(false);
        m_attachButton->setToolTip(msgAttachDebuggerTooltip());
        m_stopAction->setEnabled(false);
        m_zoomInButton->setEnabled(false);
        m_zoomOutButton->setEnabled(false);
    }
    m_formatterWidget->setVisible(m_formatterWidget->layout()->count());
}

void AppOutputPane::tabChanged(int i)
{
    const int index = indexOf(m_tabWidget->widget(i));
    if (i != -1 && index != -1) {
        enableButtons(m_runControlTabs.at(index).runControl);
    } else {
        enableDefaultButtons();
    }
}

void AppOutputPane::contextMenuRequested(const QPoint &pos, int index)
{
    QList<QAction *> actions = QList<QAction *>() << m_closeCurrentTabAction << m_closeAllTabsAction << m_closeOtherTabsAction;
    QAction *action = QMenu::exec(actions, m_tabWidget->mapToGlobal(pos), 0, m_tabWidget);
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
    RunControl *rc = qobject_cast<RunControl *>(sender());
    QTimer::singleShot(0, this, [this, rc]() { slotRunControlFinished2(rc); });
    if (rc->outputFormatter())
        rc->outputFormatter()->flush();
}

void AppOutputPane::slotRunControlFinished2(RunControl *sender)
{
    const int senderIndex = indexOf(sender);

    // This slot is queued, so the stop() call in closeTab might lead to this slot, after closeTab already cleaned up
    if (senderIndex == -1)
        return;

    // Enable buttons for current
    RunControl *current = currentRunControl();

    if (debug)
        qDebug() << "OutputPane::runControlFinished"  << sender << senderIndex
                    << " current " << current << m_runControlTabs.size();

    if (current && current == sender)
        enableButtons(current);

    ProjectExplorerPlugin::instance()->updateRunActions();

#ifdef Q_OS_WIN
    const bool isRunning = Utils::anyOf(m_runControlTabs, [](const RunControlTab &rt) {
        return rt.runControl->isRunning();
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

#include "appoutputpane.moc"

