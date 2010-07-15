/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "outputwindow.h"
#include "projectexplorerconstants.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "runconfiguration.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/icontext.h>
#include <find/basetextfind.h>
#include <aggregation/aggregate.h>

#include <QtGui/QIcon>
#include <QtGui/QScrollBar>
#include <QtGui/QTextLayout>
#include <QtGui/QTextBlock>
#include <QtGui/QPainter>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTabWidget>
#include <QtGui/QToolButton>

using namespace ProjectExplorer::Internal;
using namespace ProjectExplorer;

static const int MaxBlockCount = 100000;

OutputPane::OutputPane()
    : m_mainWidget(new QWidget)
{
    QIcon runIcon(Constants::ICON_RUN);
    runIcon.addFile(Constants::ICON_RUN_SMALL);

    // Rerun
    m_reRunButton = new QToolButton;
    m_reRunButton->setIcon(runIcon);
    m_reRunButton->setToolTip(tr("Re-run this run-configuration"));
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
    return QList<QWidget*>() << m_reRunButton << m_stopButton;
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
        if (old->sameRunConfiguration(rc) && !old->isRunning()) {
            // Reuse this tab
            delete old;
            m_outputWindows.remove(old);
            OutputWindow *ow = static_cast<OutputWindow *>(m_tabWidget->widget(i));
            ow->grayOutOldContent();
            ow->verticalScrollBar()->setValue(ow->verticalScrollBar()->maximum());
            ow->setFormatter(rc->createOutputFormatter(ow));
            m_outputWindows.insert(rc, ow);
            found = true;
            break;
        }
    }
    if (!found) {
        OutputWindow *ow = new OutputWindow(m_tabWidget);
        ow->setWindowTitle(tr("Application Output Window"));
        ow->setWindowIcon(QIcon(":/qt4projectmanager/images/window.png"));
        ow->setFormatter(rc->createOutputFormatter(ow));
        Aggregation::Aggregate *agg = new Aggregation::Aggregate;
        agg->add(ow);
        agg->add(new Find::BaseTextFind(ow));
        m_outputWindows.insert(rc, ow);
        m_tabWidget->addTab(ow, rc->displayName());
    }
}

void OutputPane::appendApplicationOutput(RunControl *rc, const QString &out,
                                         bool onStdErr)
{
    OutputWindow *ow = m_outputWindows.value(rc);
    ow->appendApplicationOutput(out, onStdErr);
}

void OutputPane::appendApplicationOutputInline(RunControl *rc,
                                               const QString &out,
                                               bool onStdErr)
{
    OutputWindow *ow = m_outputWindows.value(rc);
    ow->appendApplicationOutputInline(out, onStdErr);
}

void OutputPane::appendMessage(RunControl *rc, const QString &out, bool isError)
{
    OutputWindow *ow = m_outputWindows.value(rc);
    ow->appendMessage(out, isError);
}

void OutputPane::showTabFor(RunControl *rc)
{
    OutputWindow *ow = m_outputWindows.value(rc);
    m_tabWidget->setCurrentWidget(ow);
}

void OutputPane::reRunRunControl()
{
    int index = m_tabWidget->currentIndex();
    RunControl *rc = runControlForTab(index);
    OutputWindow *ow = static_cast<OutputWindow *>(m_tabWidget->widget(index));
    if (ProjectExplorerPlugin::instance()->projectExplorerSettings().cleanOldAppOutput)
        ow->clear();
    else
        ow->grayOutOldContent();
    ow->verticalScrollBar()->setValue(ow->verticalScrollBar()->maximum());
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
        QMessageBox messageBox(QMessageBox::Warning,
                               tr("Unable to close"),
                               tr("The application is still running."),
                               QMessageBox::Cancel | QMessageBox::Yes,
                               ow->window());
        messageBox.setInformativeText(tr("Force it to quit?"));
        messageBox.setDefaultButton(QMessageBox::Yes);
        messageBox.button(QMessageBox::Yes)->setText(tr("Force Quit"));

        if (messageBox.exec() != QMessageBox::Yes)
            return;

        rc->stop();
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
        m_reRunButton->setEnabled(!rc->isRunning());
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
        m_reRunButton->setEnabled(rc);
        m_stopAction->setEnabled(false);
    }
}

RunControl* OutputPane::runControlForTab(int index) const
{
    return m_outputWindows.key(qobject_cast<OutputWindow *>(m_tabWidget->widget(index)));
}

bool OutputPane::canNext()
{
    return false;
}

bool OutputPane::canPrevious()
{
    return false;
}

void OutputPane::goToNext()
{

}

void OutputPane::goToPrev()
{

}

bool OutputPane::canNavigate()
{
    return false;
}

/*******************/

OutputWindow::OutputWindow(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_enforceNewline(false)
    , m_scrollToBottom(false)
    , m_formatter(0)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setCenterOnScroll(false);
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);

    static uint usedIds = 0;
    Core::ICore *core = Core::ICore::instance();
    QList<int> context;
    context << core->uniqueIDManager()->uniqueIdentifier(QString(Constants::C_APP_OUTPUT) + QString().setNum(usedIds++));
    m_outputWindowContext = new Core::BaseContext(this, context);
    core->addContextObject(m_outputWindowContext);

    QAction *undoAction = new QAction(this);
    QAction *redoAction = new QAction(this);
    QAction *cutAction = new QAction(this);
    QAction *copyAction = new QAction(this);
    QAction *pasteAction = new QAction(this);
    QAction *selectAllAction = new QAction(this);

    Core::ActionManager *am = core->actionManager();
    am->registerAction(undoAction, Core::Constants::UNDO, context);
    am->registerAction(redoAction, Core::Constants::REDO, context);
    am->registerAction(cutAction, Core::Constants::CUT, context);
    am->registerAction(copyAction, Core::Constants::COPY, context);
    am->registerAction(pasteAction, Core::Constants::PASTE, context);
    am->registerAction(selectAllAction, Core::Constants::SELECTALL, context);

    connect(undoAction, SIGNAL(triggered()), this, SLOT(undo()));
    connect(redoAction, SIGNAL(triggered()), this, SLOT(redo()));
    connect(cutAction, SIGNAL(triggered()), this, SLOT(cut()));
    connect(copyAction, SIGNAL(triggered()), this, SLOT(copy()));
    connect(pasteAction, SIGNAL(triggered()), this, SLOT(paste()));
    connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAll()));

    connect(this, SIGNAL(undoAvailable(bool)), undoAction, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(redoAvailable(bool)), redoAction, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(copyAvailable(bool)), cutAction, SLOT(setEnabled(bool)));  // OutputWindow never read-only
    connect(this, SIGNAL(copyAvailable(bool)), copyAction, SLOT(setEnabled(bool)));

    undoAction->setEnabled(false);
    redoAction->setEnabled(false);
    cutAction->setEnabled(false);
    copyAction->setEnabled(false);
}

OutputWindow::~OutputWindow()
{
    Core::ICore::instance()->removeContextObject(m_outputWindowContext);
    delete m_outputWindowContext;
}

OutputFormatter *OutputWindow::formatter() const
{
    return m_formatter;
}

void OutputWindow::setFormatter(OutputFormatter *formatter)
{
    m_formatter = formatter;
    m_formatter->setPlainTextEdit(this);
}

void OutputWindow::showEvent(QShowEvent *e)
{
    QPlainTextEdit::showEvent(e);
    if (m_scrollToBottom) {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
    m_scrollToBottom = false;
}

QString OutputWindow::doNewlineEnfocement(const QString &out)
{
    m_scrollToBottom = true;
    QString s = out;
    if (m_enforceNewline)
        s.prepend(QLatin1Char('\n'));

    m_enforceNewline = true; // make appendOutputInline put in a newline next time

    if (s.endsWith(QLatin1Char('\n')))
        s.chop(1);

    return s;
}

void OutputWindow::appendApplicationOutput(const QString &out, bool onStdErr)
{
    setMaximumBlockCount(MaxBlockCount);
    const bool atBottom = isScrollbarAtBottom();
    m_formatter->appendApplicationOutput(doNewlineEnfocement(out), onStdErr);
    if (atBottom)
        scrollToBottom();
    enableUndoRedo();
}

void OutputWindow::appendApplicationOutputInline(const QString &out, bool onStdErr)
{
    m_scrollToBottom = true;
    setMaximumBlockCount(MaxBlockCount);

    int newline = -1;
    bool enforceNewline = m_enforceNewline;
    m_enforceNewline = false;
    const bool atBottom = isScrollbarAtBottom();

    if (!enforceNewline) {
        newline = out.indexOf(QLatin1Char('\n'));
        moveCursor(QTextCursor::End);
        m_formatter->appendApplicationOutput(newline < 0 ? out : out.left(newline), onStdErr); // doesn't enforce new paragraph like appendPlainText
    }

    QString s = out.mid(newline+1);
    if (s.isEmpty()) {
        m_enforceNewline = true;
    } else {
        if (s.endsWith(QLatin1Char('\n'))) {
            m_enforceNewline = true;
            s.chop(1);
        }
        m_formatter->appendApplicationOutput(QLatin1Char('\n') + s, onStdErr);
    }

    if (atBottom)
        scrollToBottom();
    enableUndoRedo();
}

void OutputWindow::appendMessage(const QString &out, bool isError)
{
    setMaximumBlockCount(MaxBlockCount);
    const bool atBottom = isScrollbarAtBottom();
    m_formatter->appendMessage(doNewlineEnfocement(out), isError);
    if (atBottom)
        scrollToBottom();
    enableUndoRedo();
}

// TODO rename
void OutputWindow::appendText(const QString &text, const QTextCharFormat &format, int maxLineCount)
{
    if (document()->blockCount() > maxLineCount)
        return;
    const bool atBottom = isScrollbarAtBottom();
    QTextCursor cursor = QTextCursor(document());
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock();
    cursor.insertText(doNewlineEnfocement(text), format);

    if (document()->blockCount() > maxLineCount) {
        QTextCharFormat tmp;
        tmp.setFontWeight(QFont::Bold);
        cursor.insertText(tr("Additional output omitted\n"), tmp);
    }

    cursor.endEditBlock();
    if (atBottom)
        scrollToBottom();
}

bool OutputWindow::isScrollbarAtBottom() const
{
    return verticalScrollBar()->value() == verticalScrollBar()->maximum();
}

void OutputWindow::scrollToBottom()
{
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void OutputWindow::grayOutOldContent()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat endFormat = cursor.charFormat();

    cursor.select(QTextCursor::Document);

    QTextCharFormat format;
    const QColor bkgColor = palette().base().color();
    const QColor fgdColor = palette().text().color();
    double bkgFactor = 0.50;
    double fgdFactor = 1.-bkgFactor;
    format.setForeground(QColor((bkgFactor * bkgColor.red() + fgdFactor * fgdColor.red()),
                             (bkgFactor * bkgColor.green() + fgdFactor * fgdColor.green()),
                             (bkgFactor * bkgColor.blue() + fgdFactor * fgdColor.blue()) ));
    cursor.mergeCharFormat(format);

    cursor.movePosition(QTextCursor::End);
    cursor.setCharFormat(endFormat);
    cursor.insertBlock(QTextBlockFormat());
}

void OutputWindow::enableUndoRedo()
{
    setMaximumBlockCount(0);
    setUndoRedoEnabled(true);
}

void OutputWindow::mousePressEvent(QMouseEvent *e)
{
    QPlainTextEdit::mousePressEvent(e);
    if (m_formatter)
        m_formatter->mousePressEvent(e);
}

void OutputWindow::mouseReleaseEvent(QMouseEvent *e)
{
    QPlainTextEdit::mouseReleaseEvent(e);
    if (m_formatter)
        m_formatter->mouseReleaseEvent(e);
}

void OutputWindow::mouseMoveEvent(QMouseEvent *e)
{
    QPlainTextEdit::mouseMoveEvent(e);
    if (m_formatter)
        m_formatter->mouseMoveEvent(e);
}
