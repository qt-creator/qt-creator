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

#include "outputwindow.h"
#include "projectexplorerconstants.h"
#include "runconfiguration.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <find/basetextfind.h>
#include <aggregation/aggregate.h>

#include <QtGui/QIcon>
#include <QtGui/QScrollBar>
#include <QtGui/QTextLayout>
#include <QtGui/QPainter>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTabWidget>

using namespace ProjectExplorer::Internal;
using namespace ProjectExplorer;

OutputPane::OutputPane()
    : m_mainWidget(new QWidget)
{
//     m_insertLineButton = new QToolButton;
//     m_insertLineButton->setIcon(QIcon(ProjectExplorer::Constants::ICON_INSERT_LINE));
//     m_insertLineButton->setText(tr("Insert line"));
//     m_insertLineButton->setToolTip(tr("Insert line"));
//     m_insertLineButton->setAutoRaise(true);
//     connect(m_insertLineButton, SIGNAL(clicked()), this, SLOT(insertLine()));

    QIcon runIcon(Constants::ICON_RUN);
    runIcon.addFile(Constants::ICON_RUN_SMALL);

    // Rerun
    m_reRunButton = new QToolButton;
    m_reRunButton->setIcon(runIcon);
    m_reRunButton->setToolTip(tr("Rerun this runconfiguration"));
    m_reRunButton->setAutoRaise(true);
    m_reRunButton->setEnabled(false);
    connect(m_reRunButton, SIGNAL(clicked()),
            this, SLOT(reRunRunControl()));

    // Stop
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    QList<int> globalcontext;
    globalcontext.append(Core::Constants::C_GLOBAL_ID);

    m_stopAction = new QAction(QIcon(Constants::ICON_STOP), tr("Stop"), this);
    m_stopAction->setToolTip(tr("Stop"));
    m_stopAction->setEnabled(false);

    Core::Command *cmd = am->registerAction(m_stopAction, Constants::STOP, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+R")));

    m_stopButton = new QToolButton;
    m_stopButton->setDefaultAction(cmd->action());
    m_stopButton->setAutoRaise(true);

    connect(m_stopAction, SIGNAL(triggered()),
            this, SLOT(stopRunControl()));

    // Spacer (?)

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    m_tabWidget = new QTabWidget;
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    layout->addWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    m_mainWidget->setLayout(layout);

    connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()),
            this, SLOT(coreAboutToClose()));
}

void OutputPane::coreAboutToClose()
{
    while (m_tabWidget->count()) {
        RunControl *rc = runControlForTab(0);
        if (rc->isRunning())
            rc->stop();
        closeTab(0);
    }
}

OutputPane::~OutputPane()
{
    delete m_mainWidget;
}

QWidget *OutputPane::outputWidget(QWidget *)
{
    return m_mainWidget;
}

QList<QWidget*> OutputPane::toolBarWidgets() const
{
    return QList<QWidget*>() << m_reRunButton << m_stopButton
            ; // << m_insertLineButton;
}

QString OutputPane::name() const
{
    return tr("Application Output");
}

int OutputPane::priorityInStatusBar() const
{
    return 60;
}

void OutputPane::clearContents()
{
    OutputWindow *currentWindow = qobject_cast<OutputWindow *>(m_tabWidget->currentWidget());
    if (currentWindow)
        currentWindow->clear();
}

void OutputPane::visibilityChanged(bool /* b */)
{
}

bool OutputPane::hasFocus()
{
    return m_tabWidget->currentWidget() && m_tabWidget->currentWidget()->hasFocus();
}

bool OutputPane::canFocus()
{
    return m_tabWidget->currentWidget();
}

void OutputPane::setFocus()
{
    if (m_tabWidget->currentWidget())
        m_tabWidget->currentWidget()->setFocus();
}

void OutputPane::appendOutput(const QString &/*out*/)
{
    // This function is in the interface, since we can't do anything sensible here, we don't do anything here.
}

void OutputPane::createNewOutputWindow(RunControl *rc)
{
    connect(rc, SIGNAL(started()),
            this, SLOT(runControlStarted()));
    connect(rc, SIGNAL(finished()),
            this, SLOT(runControlFinished()));

    // First look if we can reuse a tab
    bool found = false;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        RunControl *old = runControlForTab(i);
        if (old->runConfiguration() == rc->runConfiguration() && !old->isRunning()) {
            // Reuse this tab
            delete old;
            m_outputWindows.remove(old);
            OutputWindow *ow = static_cast<OutputWindow *>(m_tabWidget->widget(i));
            ow->appendOutput("");//New line
            m_outputWindows.insert(rc, ow);
            found = true;
            break;
        }
    }
    if (!found) {
        OutputWindow *ow = new OutputWindow(m_tabWidget);
        Aggregation::Aggregate *agg = new Aggregation::Aggregate;
        agg->add(ow);
        agg->add(new Find::BaseTextFind(ow));
        m_outputWindows.insert(rc, ow);
        m_tabWidget->addTab(ow, rc->runConfiguration()->name());
    }
}

void OutputPane::appendOutput(RunControl *rc, const QString &out)
{
    OutputWindow *ow = m_outputWindows.value(rc);
    ow->appendOutput(out);
}

void OutputPane::appendOutputInline(RunControl *rc, const QString &out)
{
    OutputWindow *ow = m_outputWindows.value(rc);
    ow->appendOutputInline(out);
}

void OutputPane::showTabFor(RunControl *rc)
{
    OutputWindow *ow = m_outputWindows.value(rc);
    m_tabWidget->setCurrentWidget(ow);
}

void OutputPane::insertLine()
{
    OutputWindow *currentWindow = qobject_cast<OutputWindow *>(m_tabWidget->currentWidget());
    if (currentWindow)
        currentWindow->clear();
}

void OutputPane::reRunRunControl()
{
    RunControl *rc = runControlForTab(m_tabWidget->currentIndex());
    if (rc->runConfiguration()->project() != 0)
        rc->start();
}

void OutputPane::stopRunControl()
{
    RunControl *rc = runControlForTab(m_tabWidget->currentIndex());
    rc->stop();
}

void OutputPane::closeTab(int index)
{
    OutputWindow *ow = static_cast<OutputWindow *>(m_tabWidget->widget(index));
    RunControl *rc = m_outputWindows.key(ow);

    if (rc->isRunning()) {
        QString msg = tr("The application is still running. Close it first.");
        QMessageBox::critical(0, tr("Unable to close"), msg);
        return;
    }

    m_tabWidget->removeTab(index);
    delete ow;
    delete rc;
}

void OutputPane::projectRemoved()
{
    tabChanged(m_tabWidget->currentIndex());
}

void OutputPane::tabChanged(int i)
{
    if (i == -1) {
        m_stopAction->setEnabled(false);
        m_reRunButton->setEnabled(false);
    } else {
        RunControl *rc = runControlForTab(i);
        m_stopAction->setEnabled(rc->isRunning());
        m_reRunButton->setEnabled(!rc->isRunning() && rc->runConfiguration()->project());
    }
}

void OutputPane::runControlStarted()
{
    RunControl *rc = runControlForTab(m_tabWidget->currentIndex());
    if (rc == qobject_cast<RunControl *>(sender())) {
        m_reRunButton->setEnabled(false);
        m_stopAction->setEnabled(true);
    }
}

void OutputPane::runControlFinished()
{
    RunControl *rc = runControlForTab(m_tabWidget->currentIndex());
    if (rc == qobject_cast<RunControl *>(sender())) {
        m_reRunButton->setEnabled(rc->runConfiguration()->project());
        m_stopAction->setEnabled(false);
    }
}

RunControl* OutputPane::runControlForTab(int index) const
{
    return m_outputWindows.key(qobject_cast<OutputWindow *>(m_tabWidget->widget(index)));
}


/*******************/

OutputWindow::OutputWindow(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setCenterOnScroll(false);
    setMaximumBlockCount(100000);
    setWindowTitle(tr("Application Output Window"));
    setWindowIcon(QIcon(":/qt4projectmanager/images/window.png"));
    setFrameShape(QFrame::NoFrame);
}


OutputWindow::~OutputWindow()
{
}

void OutputWindow::appendOutput(const QString &out)
{
    if (out.endsWith('\n'))
        appendPlainText(out);
    else
        appendPlainText(out + '\n');
}

void OutputWindow::appendOutputInline(const QString &out)
{
    moveCursor(QTextCursor::End);
    int newline = out.indexOf(QLatin1Char('\n'));
    if (newline < 0) {
        insertPlainText(out);
        return;
    }
    insertPlainText(out.left(newline));
    if (newline < out.length())
        appendPlainText(out.mid(newline+1));
}

void OutputWindow::insertLine()
{
    appendPlainText(QString());
}

