/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "appoutputpane.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "runconfiguration.h"
#include "session.h"
#include "windebuginterface.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/outputwindow.h>
#include <coreplugin/find/basetextfind.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/invoker.h>

#include <utils/algorithm.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>

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

namespace ProjectExplorer {
namespace Internal {

class TabWidget : public QTabWidget
{
    Q_OBJECT
public:
    TabWidget(QWidget *parent = 0);
signals:
    void contextMenuRequested(const QPoint &pos, const int index);
protected:
    bool eventFilter(QObject *object, QEvent *event);
private slots:
    void slotContextMenuRequested(const QPoint &pos);
private:
    int m_tabIndexForMiddleClick;
};

}
}

TabWidget::TabWidget(QWidget *parent)
    : QTabWidget(parent), m_tabIndexForMiddleClick(-1)
{
    tabBar()->installEventFilter(this);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotContextMenuRequested(QPoint)));
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
    runControl(rc), window(w), asyncClosing(false), behaviorOnOutput(Flash)
{
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
    m_attachButton(new QToolButton)
{
    setObjectName(QLatin1String("AppOutputPane")); // Used in valgrind engine

    // Rerun
    m_reRunButton->setIcon(QIcon(QLatin1String(Constants::ICON_RUN_SMALL)));
    m_reRunButton->setToolTip(tr("Re-run this run-configuration"));
    m_reRunButton->setAutoRaise(true);
    m_reRunButton->setEnabled(false);
    connect(m_reRunButton, SIGNAL(clicked()),
            this, SLOT(reRunRunControl()));

    // Stop
    QIcon stopIcon = QIcon(QLatin1String(Constants::ICON_STOP));
    stopIcon.addFile(QLatin1String(Constants::ICON_STOP_SMALL));
    m_stopAction->setIcon(stopIcon);
    m_stopAction->setToolTip(tr("Stop"));
    m_stopAction->setEnabled(false);

    Core::Command *cmd = Core::ActionManager::registerAction(m_stopAction, Constants::STOP);

    m_stopButton->setDefaultAction(cmd->action());
    m_stopButton->setAutoRaise(true);

    connect(m_stopAction, SIGNAL(triggered()),
            this, SLOT(stopRunControl()));

    // Attach
    m_attachButton->setToolTip(msgAttachDebuggerTooltip());
    m_attachButton->setEnabled(false);
    m_attachButton->setIcon(QIcon(QLatin1String(Constants::ICON_DEBUG_SMALL)));
    m_attachButton->setAutoRaise(true);

    connect(m_attachButton, SIGNAL(clicked()),
            this, SLOT(attachToRunControl()));

    // Spacer (?)

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    layout->addWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    connect(m_tabWidget, SIGNAL(contextMenuRequested(QPoint,int)), this, SLOT(contextMenuRequested(QPoint,int)));

    m_mainWidget->setLayout(layout);

    connect(SessionManager::instance(), SIGNAL(aboutToUnloadSession(QString)),
            this, SLOT(aboutToUnloadSession()));
    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateFromSettings()));

#ifdef Q_OS_WIN
    connect(this, &AppOutputPane::allRunControlsFinished,
            WinDebugInterface::instance(), &WinDebugInterface::stop);
#endif
}

AppOutputPane::~AppOutputPane()
{
    if (debug)
        qDebug() << "OutputPane::~OutputPane: Entries left" << m_runControlTabs.size();

    foreach (const RunControlTab &rt, m_runControlTabs)
        delete rt.runControl;
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
    return 0;
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
    return QList<QWidget*>() << m_reRunButton << m_stopButton << m_attachButton;
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

void AppOutputPane::createNewOutputWindow(RunControl *rc)
{
    connect(rc, SIGNAL(started()),
            this, SLOT(slotRunControlStarted()));
    connect(rc, SIGNAL(finished()),
            this, SLOT(slotRunControlFinished()));
    connect(rc, SIGNAL(applicationProcessHandleChanged()),
            this, SLOT(enableButtons()));
    connect(rc, SIGNAL(appendMessage(ProjectExplorer::RunControl*,QString,Utils::OutputFormat)),
            this, SLOT(appendMessage(ProjectExplorer::RunControl*,QString,Utils::OutputFormat)));

    Utils::OutputFormatter *formatter = rc->outputFormatter();
    formatter->setFont(TextEditor::TextEditorSettings::fontSettings().font());

    // First look if we can reuse a tab
    const int size = m_runControlTabs.size();
    for (int i = 0; i < size; i++) {
        RunControlTab &tab =m_runControlTabs[i];
        if (tab.runControl->sameRunConfiguration(rc) && !tab.runControl->isRunning()) {
            // Reuse this tab
            delete tab.runControl;
            tab.runControl = rc;
            handleOldOutput(tab.window);
            tab.window->scrollToBottom();
            tab.window->setFormatter(formatter);
            if (debug)
                qDebug() << "OutputPane::createNewOutputWindow: Reusing tab" << i << " for " << rc;
            return;
        }
    }
    // Create new
    static uint counter = 0;
    Core::Id contextId = Core::Id(Constants::C_APP_OUTPUT).withSuffix(counter++);
    Core::Context context(contextId);
    Core::OutputWindow *ow = new Core::OutputWindow(context, m_tabWidget);
    ow->setWindowTitle(tr("Application Output Window"));
    ow->setWindowIcon(QIcon(QLatin1String(Constants::ICON_WINDOW)));
    ow->setFormatter(formatter);
    ow->setWordWrapEnabled(ProjectExplorerPlugin::projectExplorerSettings().wrapAppOutput);
    ow->setMaxLineCount(ProjectExplorerPlugin::projectExplorerSettings().maxAppOutputLines);
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
    const int size = m_runControlTabs.size();
    for (int i = 0; i < size; i++) {
        RunControlTab &tab =m_runControlTabs[i];
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
    tab.runControl->start();
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
    QTC_ASSERT(index != -1 && m_runControlTabs.at(index).runControl->isRunning(), return);

    RunControl *rc = m_runControlTabs.at(index).runControl;
    if (rc->isRunning() && optionallyPromptToStop(rc))
        rc->stop();

    if (debug)
        qDebug() << "OutputPane::stopRunControl " << rc;
}

bool AppOutputPane::closeTabs(CloseTabMode mode)
{
    bool allClosed = true;
    for (int t = m_tabWidget->count() - 1; t >= 0; t--)
        if (!closeTab(t, mode))
            allClosed = false;
    if (debug)
        qDebug() << "OutputPane::closeTabs() returns " << allClosed;
    return allClosed;
}

bool AppOutputPane::closeTab(int index)
{
    return closeTab(index, CloseTabWithPrompt);
}

bool AppOutputPane::closeTab(int tabIndex, CloseTabMode closeTabMode)
{
    int index = indexOf(m_tabWidget->widget(tabIndex));
    QTC_ASSERT(index != -1, return true);

    if (debug)
        qDebug() << "OutputPane::closeTab tab " << tabIndex << m_runControlTabs[index].runControl
                        << m_runControlTabs[index].window << m_runControlTabs[index].asyncClosing;
    // Prompt user to stop
    if (m_runControlTabs[index].runControl->isRunning()) {
        switch (closeTabMode) {
        case CloseTabNoPrompt:
            break;
        case CloseTabWithPrompt:
            QWidget *tabWidget = m_tabWidget->widget(tabIndex);
            if (!m_runControlTabs[index].runControl->promptToStop())
                return false;
            // The event loop has run, thus the ordering might have changed, a tab might
            // have been closed, so do some strange things...
            tabIndex = m_tabWidget->indexOf(tabWidget);
            index = indexOf(tabWidget);
            if (tabIndex == -1 || index == -1)
                return false;
            break;
        }
        if (m_runControlTabs[index].runControl->isRunning()) { // yes it might have stopped already, then just close
            QWidget *tabWidget = m_tabWidget->widget(tabIndex);
            if (m_runControlTabs[index].runControl->stop() == RunControl::AsynchronousStop) {
                m_runControlTabs[index].asyncClosing = true;
                return false;
            }
            tabIndex = m_tabWidget->indexOf(tabWidget);
            index = indexOf(tabWidget);
            if (tabIndex == -1 || index == -1)
                return false;
        }
    }

    m_tabWidget->removeTab(tabIndex);
    delete m_runControlTabs[index].runControl;
    delete m_runControlTabs[index].window;
    m_runControlTabs.removeAt(index);
    updateCloseActions();

    if (m_runControlTabs.isEmpty())
        hide();

    return true;
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

void AppOutputPane::enableButtons()
{
    const RunControl *rc = currentRunControl();
    const bool isRunning = rc && rc->isRunning();
    enableButtons(rc, isRunning);
}

void AppOutputPane::enableButtons(const RunControl *rc /* = 0 */, bool isRunning /*  = false */)
{
    if (rc) {
        m_reRunButton->setEnabled(!isRunning && rc->supportsReRunning());
        m_reRunButton->setIcon(QIcon(rc->icon()));
        m_stopAction->setEnabled(isRunning);
        if (isRunning && debuggerPlugin() && rc->applicationProcessHandle().isValid()) {
            m_attachButton->setEnabled(true);
            m_attachButton->setToolTip(msgAttachDebuggerTooltip(rc->applicationProcessHandle().toString()));
        } else {
            m_attachButton->setEnabled(false);
            m_attachButton->setToolTip(msgAttachDebuggerTooltip());
        }
    } else {
        m_reRunButton->setEnabled(false);
        m_reRunButton->setIcon(QIcon(QLatin1String(Constants::ICON_RUN_SMALL)));
        m_attachButton->setEnabled(false);
        m_attachButton->setToolTip(msgAttachDebuggerTooltip());
        m_stopAction->setEnabled(false);
    }
}

void AppOutputPane::tabChanged(int i)
{
    const int index = indexOf(m_tabWidget->widget(i));
    if (i != -1) {
        const RunControl *rc = m_runControlTabs.at(index).runControl;
        enableButtons(rc, rc->isRunning());
    } else {
        enableButtons();
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

void AppOutputPane::slotRunControlStarted()
{
    RunControl *current = currentRunControl();
    if (current && current == sender())
        enableButtons(current, true); // RunControl::isRunning() cannot be trusted in signal handler.
    emit runControlStarted(current);
}

void AppOutputPane::slotRunControlFinished()
{
    RunControl *rc = qobject_cast<RunControl *>(sender());
    QMetaObject::invokeMethod(this, "slotRunControlFinished2", Qt::QueuedConnection,
                              Q_ARG(ProjectExplorer::RunControl *, rc));
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
        enableButtons(current, false); // RunControl::isRunning() cannot be trusted in signal handler.

    // Check for asynchronous close. Close the tab.
    if (m_runControlTabs.at(senderIndex).asyncClosing)
        closeTab(tabWidgetIndexOf(senderIndex), CloseTabNoPrompt);

    emit runControlFinished(sender);

    if (!isRunning())
        emit allRunControlsFinished();
}

bool AppOutputPane::isRunning() const
{
    return Utils::anyOf(m_runControlTabs, [](const RunControlTab &rt) {
        return rt.runControl->isRunning();
    });
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

