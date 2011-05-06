/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "appoutputpane.h"
#include "projectexplorerconstants.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "runconfiguration.h"
#include "session.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <find/basetextfind.h>
#include <aggregation/aggregate.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <utils/qtcassert.h>
#include <utils/outputformatter.h>

#include <QtGui/QAction>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTabWidget>
#include <QtGui/QToolButton>

#include <QtCore/QDebug>

enum { debug = 0 };

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

AppOutputPane::RunControlTab::RunControlTab(RunControl *rc, Core::OutputWindow *w) :
    runControl(rc), window(w), asyncClosing(false)
{
}

AppOutputPane::AppOutputPane() :
    m_mainWidget(new QWidget),
    m_tabWidget(new QTabWidget),
    m_stopAction(new QAction(QIcon(QLatin1String(Constants::ICON_STOP)), tr("Stop"), this)),
    m_reRunButton(new QToolButton),
    m_stopButton(new QToolButton)
{
    // Rerun
    m_reRunButton->setIcon(QIcon(ProjectExplorer::Constants::ICON_RUN_SMALL));
    m_reRunButton->setToolTip(tr("Re-run this run-configuration"));
    m_reRunButton->setAutoRaise(true);
    m_reRunButton->setEnabled(false);
    connect(m_reRunButton, SIGNAL(clicked()),
            this, SLOT(reRunRunControl()));

    // Stop
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::Context globalcontext(Core::Constants::C_GLOBAL);

    m_stopAction->setToolTip(tr("Stop"));
    m_stopAction->setEnabled(false);

    Core::Command *cmd = am->registerAction(m_stopAction, Constants::STOP, globalcontext);

    m_stopButton->setDefaultAction(cmd->action());
    m_stopButton->setAutoRaise(true);

    connect(m_stopAction, SIGNAL(triggered()),
            this, SLOT(stopRunControl()));

    // Spacer (?)

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    layout->addWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    m_mainWidget->setLayout(layout);

    connect(ProjectExplorerPlugin::instance()->session(), SIGNAL(aboutToUnloadSession()),
            this, SLOT(aboutToUnloadSession()));
    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateFromSettings()));
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

bool AppOutputPane::aboutToClose() const
{
    foreach (const RunControlTab &rt, m_runControlTabs)
        if (rt.runControl->isRunning() && !rt.runControl->promptToStop())
            return false;
    return true;
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
    return QList<QWidget*>() << m_reRunButton << m_stopButton;
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

bool AppOutputPane::hasFocus()
{
    return m_tabWidget->currentWidget() && m_tabWidget->currentWidget()->hasFocus();
}

bool AppOutputPane::canFocus()
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
            this, SLOT(runControlStarted()));
    connect(rc, SIGNAL(finished()),
            this, SLOT(runControlFinished()));
    connect(rc, SIGNAL(appendMessage(ProjectExplorer::RunControl*,QString,Utils::OutputFormat)),
            this, SLOT(appendMessage(ProjectExplorer::RunControl*,QString,Utils::OutputFormat)));

    Utils::OutputFormatter *formatter = rc->outputFormatter();
    formatter->setFont(TextEditor::TextEditorSettings::instance()->fontSettings().font());

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
    Core::Context context(Constants::C_APP_OUTPUT, counter++);
    Core::OutputWindow *ow = new Core::OutputWindow(context, m_tabWidget);
    ow->setWindowTitle(tr("Application Output Window"));
    // TODO the following is a hidden impossible dependency of projectexplorer on qt4projectmanager
    ow->setWindowIcon(QIcon(QLatin1String(Qt4ProjectManager::Constants::ICON_WINDOW)));
    ow->setFormatter(formatter);
    ow->setWordWrapEnabled(ProjectExplorerPlugin::instance()->projectExplorerSettings().wrapAppOutput);
    ow->setMaxLineCount(ProjectExplorerPlugin::instance()->projectExplorerSettings().maxAppOutputLines);
    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(ow);
    agg->add(new Find::BaseTextFind(ow));
    m_runControlTabs.push_back(RunControlTab(rc, ow));
    m_tabWidget->addTab(ow, rc->displayName());
    if (debug)
        qDebug() << "OutputPane::createNewOutputWindow: Adding tab for " << rc;
}

void AppOutputPane::handleOldOutput(Core::OutputWindow *window) const
{
    if (ProjectExplorerPlugin::instance()->projectExplorerSettings().cleanOldAppOutput)
        window->clear();
    else
        window->grayOutOldContent();
}

void AppOutputPane::updateFromSettings()
{
    const int size = m_runControlTabs.size();
    for (int i = 0; i < size; i++) {
        RunControlTab &tab =m_runControlTabs[i];
        tab.window->setWordWrapEnabled(ProjectExplorerPlugin::instance()->projectExplorerSettings().wrapAppOutput);
        tab.window->setMaxLineCount(ProjectExplorerPlugin::instance()->projectExplorerSettings().maxAppOutputLines);
    }
}

void AppOutputPane::appendMessage(RunControl *rc, const QString &out, Utils::OutputFormat format)
{
    const int index = indexOf(rc);
    if (index != -1)
        m_runControlTabs.at(index).window->appendMessage(out, format);
}

void AppOutputPane::showTabFor(RunControl *rc)
{
    m_tabWidget->setCurrentIndex(tabWidgetIndexOf(indexOf(rc)));
}

void AppOutputPane::reRunRunControl()
{
    const int index = currentIndex();
    QTC_ASSERT(index != -1 && !m_runControlTabs.at(index).runControl->isRunning(), return;)

    RunControlTab &tab = m_runControlTabs[index];

    handleOldOutput(tab.window);
    tab.window->scrollToBottom();
    tab.runControl->start();
}

void AppOutputPane::stopRunControl()
{
    const int index = currentIndex();
    QTC_ASSERT(index != -1 && m_runControlTabs.at(index).runControl->isRunning(), return;)

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
    const int index = indexOf(m_tabWidget->widget(tabIndex));
    QTC_ASSERT(index != -1, return true;)

    RunControlTab &tab = m_runControlTabs[index];

    if (debug)
            qDebug() << "OutputPane::closeTab tab " << tabIndex << tab.runControl
                        << tab.window << tab.asyncClosing;
    // Prompt user to stop
    if (tab.runControl->isRunning()) {
        switch (closeTabMode) {
        case CloseTabNoPrompt:
            break;
        case CloseTabWithPrompt:
            if (!tab.runControl->promptToStop())
                return false;
            break;
        }
        if (tab.runControl->stop() == RunControl::AsynchronousStop) {
            tab.asyncClosing = true;
            return false;
        }
    }

    m_tabWidget->removeTab(tabIndex);
    if (tab.asyncClosing) { // We were invoked from its finished() signal.
        tab.runControl->deleteLater();
    } else {
        delete tab.runControl;
    }
    delete tab.window;
    m_runControlTabs.removeAt(index);
    return true;
}

bool AppOutputPane::optionallyPromptToStop(RunControl *runControl)
{
    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    ProjectExplorerSettings settings = pe->projectExplorerSettings();
    if (!runControl->promptToStop(&settings.prompToStopRunControl))
        return false;
    pe->setProjectExplorerSettings(settings);
    return true;
}

void AppOutputPane::projectRemoved()
{
    tabChanged(m_tabWidget->currentIndex());
}

void AppOutputPane::tabChanged(int i)
{
    if (i == -1) {
        m_stopAction->setEnabled(false);
        m_reRunButton->setEnabled(false);
    } else {
        const int index = indexOf(m_tabWidget->widget(i));
        QTC_ASSERT(index != -1, return; )

        RunControl *rc = m_runControlTabs.at(index).runControl;
        m_stopAction->setEnabled(rc->isRunning());
        m_reRunButton->setEnabled(!rc->isRunning());
        m_reRunButton->setIcon(rc->icon());
    }
}

void AppOutputPane::runControlStarted()
{
    RunControl *current = currentRunControl();
    if (current && current == sender()) {
        m_reRunButton->setEnabled(false);
        m_stopAction->setEnabled(true);
        m_reRunButton->setIcon(current->icon());
    }
}

void AppOutputPane::runControlFinished()
{
    RunControl *senderRunControl = qobject_cast<RunControl *>(sender());
    const int senderIndex = indexOf(senderRunControl);

    QTC_ASSERT(senderIndex != -1, return; )

    // Enable buttons for current
    RunControl *current = currentRunControl();

    if (debug)
        qDebug() << "OutputPane::runControlFinished"  << senderRunControl << senderIndex
                    << " current " << current << m_runControlTabs.size();

    if (current && current == sender()) {
        m_reRunButton->setEnabled(true);
        m_stopAction->setEnabled(false);
        m_reRunButton->setIcon(current->icon());
    }
    // Check for asynchronous close. Close the tab.
    if (m_runControlTabs.at(senderIndex).asyncClosing)
        closeTab(tabWidgetIndexOf(senderIndex), CloseTabNoPrompt);

    if (!isRunning())
        emit allRunControlsFinished();
}

bool AppOutputPane::isRunning() const
{
    foreach (const RunControlTab &rt, m_runControlTabs)
        if (rt.runControl->isRunning())
            return true;
    return false;
}

bool AppOutputPane::canNext()
{
    return false;
}

bool AppOutputPane::canPrevious()
{
    return false;
}

void AppOutputPane::goToNext()
{

}

void AppOutputPane::goToPrev()
{

}

bool AppOutputPane::canNavigate()
{
    return false;
}
